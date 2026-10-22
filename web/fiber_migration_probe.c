#define _POSIX_C_SOURCE 200809L
#include "portable_fiber.h"
#include <emscripten.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
void migration_set_tls(int);
int migration_get_tls(void);
uintptr_t migration_native_thread(void);
void migration_set_root(emscripten_fiber_t *);
emscripten_fiber_t *migration_get_root(void);
static emscripten_fiber_t roots[2],task;
static atomic_uint stage,failures,checks,finished;
static uintptr_t native_a,native_b;
static pthread_t workers[2];
static char report[384];
static void check(int value){atomic_fetch_add(&checks,1);if(!value)atomic_fetch_add(&failures,1);}
EM_JS(void,seed_other_history,(void),{
    Array.from(Asyncify.funcWrappers.keys()).reverse().forEach(function(fn){Asyncify.getCallStackId(fn);});
});
static void swap(emscripten_fiber_t *from,emscripten_fiber_t *to) {
#ifdef WEB_FIBER_BASELINE
    emscripten_fiber_swap(from,to);
#else
    if(!web_portable_fiber_swap(from,to))abort();
#endif
}
static void wait_stage(unsigned wanted) {
    struct timespec start,now,pause={0,1000000};clock_gettime(CLOCK_MONOTONIC,&start);
    do {
        if(atomic_load(&stage)>=wanted)return;
        nanosleep(&pause,NULL);clock_gettime(CLOCK_MONOTONIC,&now);
    }while(now.tv_sec-start.tv_sec<5);
    abort();
}
static void nested(unsigned depth) {
    volatile uint64_t saved[8];for(unsigned i=0;i<8;i++)saved[i]=UINT64_C(0x1234567800000000)+depth*100+i;
    if(depth)nested(depth-1);
    else {
        jmp_buf local;
        int result=setjmp(local);
        if(!result) {
            check(migration_get_tls()==11&&migration_native_thread()==native_a);
            atomic_store(&stage,1);swap(&task,migration_get_root());
            check(migration_get_tls()==22&&migration_native_thread()==native_b&&native_a!=native_b);
            atomic_store(&stage,3);swap(&task,migration_get_root());
            check(migration_get_tls()==11&&migration_native_thread()==native_a);
            longjmp(local,37);
        } else check(result==37&&migration_native_thread()==native_a);
    }
    for(unsigned i=0;i<8;i++)check(saved[i]==UINT64_C(0x1234567800000000)+depth*100+i);
}
static void fiber_entry(void *unused) {
    (void)unused;nested(8);atomic_store(&stage,5);swap(&task,migration_get_root());abort();
}
__attribute__((noinline)) static void publish(void) {
    snprintf(report,sizeof report,"{\"ok\":%s,\"checks\":%u,\"failures\":%u,\"stage\":%u,\"distinctNativeThreads\":%s,\"nestedDepth\":8,\"oppositeRewindHistories\":true}",
        atomic_load(&failures)?"false":"true",atomic_load(&checks),atomic_load(&failures),atomic_load(&stage),native_a!=native_b?"true":"false");
    MAIN_THREAD_ASYNC_EM_ASM({Module['onMigrationReport'](UTF8ToString($0));},report);
}
static void *worker_a(void *unused) {
    (void)unused;native_a=migration_native_thread();migration_set_tls(11);migration_set_root(&roots[0]);
    emscripten_fiber_init_from_current_context(&roots[0],aligned_alloc(16,262144),262144);
    swap(&roots[0],&task);check(atomic_load(&stage)==1);atomic_store(&stage,2);
    wait_stage(4);swap(&roots[0],&task);check(atomic_load(&stage)==5);atomic_store(&stage,6);
    if(atomic_fetch_add(&finished,1)==1)publish();pthread_exit(NULL);
}
static void *worker_b(void *unused) {
    (void)unused;native_b=migration_native_thread();migration_set_tls(22);migration_set_root(&roots[1]);
    seed_other_history();emscripten_fiber_init_from_current_context(&roots[1],aligned_alloc(16,262144),262144);
    wait_stage(2);swap(&roots[1],&task);check(atomic_load(&stage)==3);atomic_store(&stage,4);wait_stage(6);
    if(atomic_fetch_add(&finished,1)==1)publish();pthread_exit(NULL);
}
int migration_start(void) {
    emscripten_fiber_init(&task,fiber_entry,NULL,aligned_alloc(16,262144),262144,aligned_alloc(16,262144),262144);
    return pthread_create(&workers[0],NULL,worker_a,NULL)||pthread_create(&workers[1],NULL,worker_b,NULL);
}
int migration_join(void){return pthread_join(workers[0],NULL)||pthread_join(workers[1],NULL);}
