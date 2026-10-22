#define _POSIX_C_SOURCE 200809L
#include "runtime.h"
#include "host_semaphore.h"
#include <emscripten.h>
#include <pthread.h>
#include <sched.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static pthread_t driver,workers[2];
static atomic_uint checks,failures,entered,acquired;
static _Thread_local jmp_buf *catcher;
static _Thread_local const char *last_trap;
static char report[384];
static const uint64_t KEY=UINT64_C(0x8000001000);
void guest_trap(const char *what,uint64_t address){(void)address;if(catcher){last_trap=what;longjmp(*catcher,1);}fprintf(stderr,"semaphore fixture: %s\n",what);abort();}
static void check(int ok){atomic_fetch_add(&checks,1);if(!ok)atomic_fetch_add(&failures,1);}
static uint64_t invoke(const char *name,uint64_t key,uint64_t a,uint64_t b){
    Ctx c={0};c.x[0]=key;c.x[1]=a;c.x[2]=b;if(!web_semaphore_import(&c,name))abort();return c.x[0];
}
static void error(const char *name,uint64_t key,uint64_t a,uint64_t b,const char *text){
    jmp_buf scope;catcher=&scope;
    if(!setjmp(scope)){invoke(name,key,a,b);check(0);}else check(last_trap&&strstr(last_trap,text));
    catcher=NULL;
}
static void *waiter(void *unused){(void)unused;atomic_fetch_add(&entered,1);invoke("nnosAcquireSemaphore",KEY,0,0);atomic_fetch_add(&acquired,1);return NULL;}
__attribute__((noinline)) static void publish(void){MAIN_THREAD_ASYNC_EM_ASM({Module['onSemaphoreReport'](UTF8ToString($0));},report);}
static void *run(void *unused){
    (void)unused;
    error("nnosInitializeSemaphore",KEY,2,1,"invalid semaphore counts");
    error("nnosInitializeSemaphore",KEY,UINT64_MAX,1,"invalid semaphore counts");
    error("nnosInitializeSemaphore",KEY,0,0,"invalid semaphore counts");
    invoke("_ZN2nn2os19InitializeSemaphoreEPNS0_13SemaphoreTypeEii",KEY,2,3);
    error("nnosInitializeSemaphore",KEY,0,1,"already initialized");
    check(invoke("nnosGetCurrentSemaphoreCount",KEY,0,0)==2);
    check(invoke("_ZN2nn2os19TryAcquireSemaphoreEPNS0_13SemaphoreTypeE",KEY,0,0)==1);
    invoke("_ZN2nn2os16AcquireSemaphoreEPNS0_13SemaphoreTypeE",KEY,0,0);
    check(invoke("nnosTryAcquireSemaphore",KEY,0,0)==0);
    check(invoke("nnosTimedAcquireSemaphore",KEY,0,0)==0);
    check(invoke("nnosTimedAcquireSemaphore",KEY,UINT64_MAX,0)==0);
    struct timespec before,after;clock_gettime(CLOCK_MONOTONIC,&before);
    check(invoke("_ZN2nn2os21TimedAcquireSemaphoreEPNS0_13SemaphoreTypeENS_8TimeSpanE",KEY,2000000,0)==0);
    clock_gettime(CLOCK_MONOTONIC,&after);
    int64_t elapsed=(after.tv_sec-before.tv_sec)*INT64_C(1000000000)+after.tv_nsec-before.tv_nsec;
    check(elapsed>=1000000);
    invoke("_ZN2nn2os16ReleaseSemaphoreEPNS0_13SemaphoreTypeEi",KEY,2,0);
    invoke("_ZN2nn2os16ReleaseSemaphoreEPNS0_13SemaphoreTypeE",KEY,0,0);
    error("nnosReleaseSemaphore",KEY,0,0,"exceeds maximum");
    check(invoke("_ZN2nn2os24GetCurrentSemaphoreCountEPKNS0_13SemaphoreTypeE",KEY,0,0)==3);
    error("nnosReleaseSemaphoreCount",KEY,0,0,"exceeds maximum");
    error("nnosReleaseSemaphoreCount",KEY,UINT64_MAX,0,"exceeds maximum");
    invoke("_ZN2nn2os17FinalizeSemaphoreEPNS0_13SemaphoreTypeE",KEY,0,0);
    error("nnosTryAcquireSemaphore",KEY,0,0,"unknown semaphore");
    invoke("nnosInitializeSemaphore",KEY,1,1);
    check(invoke("nnosTimedAcquireSemaphore",KEY,UINT64_MAX,0)==1);
    invoke("nnosFinalizeSemaphore",KEY,0,0);
    invoke("nnosInitializeSemaphore",KEY,0,2);
    check(pthread_create(&workers[0],NULL,waiter,NULL)==0);check(pthread_create(&workers[1],NULL,waiter,NULL)==0);
    while(atomic_load(&entered)!=2)sched_yield();
    check(invoke("nnosGetCurrentSemaphoreCount",KEY,0,0)==0);
    invoke("nnosReleaseSemaphoreCount",KEY,2,0);
    check(pthread_join(workers[0],NULL)==0);check(pthread_join(workers[1],NULL)==0);
    check(atomic_load(&acquired)==2);check(invoke("nnosGetCurrentSemaphoreCount",KEY,0,0)==0);
    invoke("nnosFinalizeSemaphore",KEY,0,0);
    for(unsigned i=0;i<700;i++)invoke("nnosInitializeSemaphore",KEY+i*64,0,1);
    check(web_semaphore_live_count()==700);
    for(unsigned i=0;i<700;i++){
        uint64_t key=KEY+i*64;invoke("nnosReleaseSemaphore",key,0,0);
        check(invoke("nnosTryAcquireSemaphore",key,0,0)==1);check(invoke("nnosTryAcquireSemaphore",key,0,0)==0);
        invoke("nnosFinalizeSemaphore",key,0,0);
    }
    check(web_semaphore_live_count()==0);
    snprintf(report,sizeof report,"{\"ok\":%s,\"checks\":%u,\"failures\":%u,\"acquired\":%u,\"live\":%u,\"timedWaitNs\":%lld}",
      atomic_load(&failures)?"false":"true",atomic_load(&checks),atomic_load(&failures),atomic_load(&acquired),web_semaphore_live_count(),(long long)elapsed);
    publish();return NULL;
}
int semaphore_probe_start(void){return pthread_create(&driver,NULL,run,NULL);}
int semaphore_probe_join(void){return pthread_join(driver,NULL);}
