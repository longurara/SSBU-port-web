#define _POSIX_C_SOURCE 200809L
#include "runtime.h"
#include "host_mutex.h"
#include <emscripten.h>
#include <pthread.h>
#include <sched.h>
#include <setjmp.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GuestMemory memory;
static pthread_t driver,peer;
static atomic_uint checks,failures,entered;
static unsigned counter;
static char report[512];
static _Thread_local jmp_buf *catcher;
static _Thread_local const char *last_trap;
static const uint64_t BASE=UINT64_C(0x7100000000),ATTR=UINT64_C(0x7100000080),SHARED=UINT64_C(0x7100000800);
GuestMemory *elf_memory(void){return &memory;}
void guest_trap(const char *message,uint64_t address){
    (void)address;if(catcher){last_trap=message;longjmp(*catcher,1);}fprintf(stderr,"mutex fixture: %s\n",message);abort();
}
static void check(int ok){atomic_fetch_add(&checks,1);if(!ok)atomic_fetch_add(&failures,1);}
static uint64_t invoke(const char *name,uint64_t key,uint64_t argument){
    Ctx c={0};c.x[0]=key;c.x[1]=argument;
    if(!web_mutex_import(&c,name))guest_trap("mutex fixture import unsupported",key);return c.x[0];
}
__attribute__((noinline)) static void publish(void){
    MAIN_THREAD_ASYNC_EM_ASM({Module['onMutexReport'](UTF8ToString($0));},report);
}
static void *contend(void *unused){
    (void)unused;web_memory_bind(&memory);
    check(invoke("pthread_mutex_unlock",SHARED,0)==EPERM);
    atomic_store(&entered,1);
    check(invoke("pthread_mutex_lock",SHARED,0)==0);counter++;
    check(invoke("pthread_mutex_unlock",SHARED,0)==0);
    for(unsigned i=0;i<10000;i++){
        check(invoke("pthread_mutex_lock",SHARED,0)==0);counter++;
        check(invoke("pthread_mutex_unlock",SHARED,0)==0);
    }
    web_memory_bind(NULL);return NULL;
}
static void *run(void *unused){
    (void)unused;gm_init(&memory);check(gm_reserve(&memory,BASE,1048576)==GM_OK);
    check(gm_commit(&memory,BASE,262144)==GM_OK);web_memory_bind(&memory);
    check(invoke("pthread_mutexattr_init",ATTR,0)==0);
    check(invoke("pthread_mutexattr_settype",ATTR,3)==EINVAL);
    check(invoke("pthread_mutexattr_settype",ATTR,1)==0);
    check(invoke("pthread_mutexattr_gettype",ATTR,ATTR+4)==0&&rd32(ATTR+4)==1);
    uint64_t key=BASE+0x200;
    check(invoke("pthread_mutex_init",key,ATTR)==0);
    check(invoke("pthread_mutex_lock",key,0)==0);check(invoke("pthread_mutex_trylock",key,0)==0);
    check(invoke("pthread_mutex_destroy",key,0)==EBUSY);
    check(invoke("pthread_mutex_unlock",key,0)==0);check(invoke("pthread_mutex_unlock",key,0)==0);
    check(invoke("pthread_mutex_unlock",key,0)==EPERM);check(invoke("pthread_mutex_destroy",key,0)==0);
    check(invoke("pthread_mutexattr_settype",ATTR,2)==0);
    check(invoke("pthread_mutex_init",key,ATTR)==0);check(invoke("pthread_mutex_lock",key,0)==0);
    check(invoke("pthread_mutex_lock",key,0)==EDEADLK);check(invoke("pthread_mutex_trylock",key,0)==EBUSY);
    check(invoke("pthread_mutex_unlock",key,0)==0);check(invoke("pthread_mutex_destroy",key,0)==0);
    check(invoke("pthread_mutexattr_destroy",ATTR,0)==0);
    check(invoke("pthread_mutex_lock",0,0)==EINVAL);
    check(invoke("pthread_mutex_unlock",BASE+0x700,0)==EINVAL);
    invoke("_ZN2nn2os15InitializeMutexEPNS0_9MutexTypeEbi",key,1);
    invoke("_ZN2nn2os9LockMutexEPNS0_9MutexTypeE",key,0);
    check(invoke("_ZN2nn2os12TryLockMutexEPNS0_9MutexTypeE",key,0)==1);
    invoke("_ZN2nn2os11UnlockMutexEPNS0_9MutexTypeE",key,0);
    invoke("_ZN2nn2os11UnlockMutexEPNS0_9MutexTypeE",key,0);
    invoke("_ZN2nn2os13FinalizeMutexEPNS0_9MutexTypeE",key,0);
    jmp_buf caught;catcher=&caught;
    if(!setjmp(caught)){invoke("_ZN2nn2os11UnlockMutexEPNS0_9MutexTypeE",key,0);check(0);}
    else check(last_trap&&strstr(last_trap,"unknown or unowned"));
    catcher=NULL;
    // Exceed the previous fixed 512-object limit, then destroy/reuse all keys.
    for(unsigned i=0;i<1536;i++)check(invoke("pthread_mutex_init",BASE+0x1000+i*64,0)==0);
    check(web_mutex_live_count()==1536);
    for(unsigned i=0;i<1536;i++){
        uint64_t current=BASE+0x1000+i*64;
        check(invoke("pthread_mutex_trylock",current,0)==0);
        check(invoke("pthread_mutex_unlock",current,0)==0);
        check(invoke("pthread_mutex_destroy",current,0)==0);
    }
    check(web_mutex_live_count()==0);
    for(unsigned i=0;i<100;i++){
        check(invoke("pthread_mutex_init",key,0)==0);check(invoke("pthread_mutex_destroy",key,0)==0);
    }
    // Real cross-pthread exclusion, invalid-owner rejection and busy destroy.
    check(invoke("pthread_mutex_lock",SHARED,0)==0);
    check(pthread_create(&peer,NULL,contend,NULL)==0);
    while(!atomic_load(&entered))sched_yield();
    check(invoke("pthread_mutex_destroy",SHARED,0)==EBUSY);
    check(invoke("pthread_mutex_unlock",SHARED,0)==0);
    for(unsigned i=0;i<10000;i++){
        check(invoke("pthread_mutex_lock",SHARED,0)==0);counter++;
        check(invoke("pthread_mutex_unlock",SHARED,0)==0);
    }
    check(pthread_join(peer,NULL)==0);check(counter==20001);
    check(invoke("pthread_mutex_destroy",SHARED,0)==0);check(web_mutex_live_count()==0);
    snprintf(report,sizeof report,"{\"ok\":%s,\"checks\":%u,\"failures\":%u,\"peakMutexes\":%u,\"liveMutexes\":%u,\"counter\":%u}",
      atomic_load(&failures)?"false":"true",atomic_load(&checks),atomic_load(&failures),web_mutex_peak_count(),web_mutex_live_count(),counter);
    publish();
    web_memory_bind(NULL);return NULL;
}
int mutex_probe_start(void){return pthread_create(&driver,NULL,run,NULL);}
int mutex_probe_join(void){return pthread_join(driver,NULL);}
