/* Self-authored callbacks through the same FiberType adapter as the game. */
#define _POSIX_C_SOURCE 200809L
#include "host_fiber.h"
#include "runtime_frame.h"
#include <emscripten.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
uintptr_t migration_native_thread(void);
void migration_set_tls(int);
int migration_get_tls(void);
typedef struct {jmp_buf boundary;const char *error;uint64_t address;} Frame;
static atomic_uint stage,checks,failures,finished;
static pthread_t workers[2];
static uintptr_t native_a,native_b;
static unsigned worker_a_id,worker_b_id;
EM_JS(unsigned,worker_id,(void),{return workerID;});
static char report[512];
static const uint64_t KEY=UINT64_C(0x7100001000),TP_A=UINT64_C(0x7800010000),TP_B=UINT64_C(0x7800020000);
#define INITIALIZE "_ZN2nn2os15InitializeFiberEPNS0_9FiberTypeEPFS2_PvES3_S3_mi"
#define SWITCH "_ZN2nn2os13SwitchToFiberEPNS0_9FiberTypeE"
#define CURRENT "_ZN2nn2os15GetCurrentFiberEv"
#define FINALIZE "_ZN2nn2os13FinalizeFiberEPNS0_9FiberTypeE"
GuestMemory *elf_memory(void){return NULL;}
static void check(int value){atomic_fetch_add(&checks,1);if(!value)atomic_fetch_add(&failures,1);}
void guest_trap(const char *what,uint64_t address){
    Frame *frame=web_runtime_frame_get();if(!frame)abort();frame->error=what;frame->address=address;longjmp(frame->boundary,1);
}
static uint64_t current(Ctx *context){Ctx scratch=*context;check(web_fiber_import(&scratch,CURRENT));return scratch.x[0];}
static void invoke(Ctx *context,const char *name,uint64_t key){context->x[0]=key;check(web_fiber_import(context,name));}
static void expect_error(Ctx *context,const char *name,uint64_t key,const char *message){
    Frame *frame=calloc(1,sizeof *frame);if(!frame)abort();void *previous=web_runtime_frame_get();web_runtime_frame_set(frame);
    if(!setjmp(frame->boundary)){invoke(context,name,key);check(0);}else check(frame->error&&!strcmp(frame->error,message));
    web_runtime_frame_set(previous);free(frame);
}
static void wait_stage(unsigned wanted){
    struct timespec start,now,pause={0,1000000};clock_gettime(CLOCK_MONOTONIC,&start);
    do{if(atomic_load(&stage)>=wanted)return;nanosleep(&pause,NULL);clock_gettime(CLOCK_MONOTONIC,&now);}while(now.tv_sec-start.tv_sec<5);
    abort();
}
__attribute__((noinline)) static int valid_varargs(void){
    return EM_ASM_INT({return $0===0x31a5&&$1===12.5&&$2===0x123456789abcdefn?1:0;},
      0x31a5,12.5,UINT64_C(0x0123456789abcdef));
}
static void nested(Ctx *context,unsigned depth){
    volatile uint64_t locals[8];for(unsigned i=0;i<8;i++)locals[i]=UINT64_C(0xabcdef0000000000)+depth*100+i;
    if(depth)nested(context,depth-1);
    else{
        check(valid_varargs());
        check(context->x[0]==0x1234&&context->x[31]==UINT64_C(0x7100200000)&&context->tpidr==TP_A);
        check(current(context)==KEY&&migration_native_thread()==native_a&&migration_get_tls()==11);
        context->x[17]=UINT64_C(0xfeedfacecafebeef);context->v[7].d[1]=UINT64_C(0x0123456789abcdef);
        expect_error(context,SWITCH,KEY,"switch to current fiber");
        expect_error(context,FINALIZE,KEY,"finalizing unknown or running fiber");
        Frame *caught=calloc(1,sizeof *caught);if(!caught)abort();void *previous=web_runtime_frame_get();web_runtime_frame_set(caught);
        if(!setjmp(caught->boundary)){
            atomic_store(&stage,1);invoke(context,SWITCH,0);
            check(valid_varargs());
            check(context->tpidr==TP_B&&migration_native_thread()==native_b&&migration_get_tls()==22);
            guest_trap("diagnostic from destination pthread",38);
        }else check(!strcmp(caught->error,"diagnostic from destination pthread")&&caught->address==38&&migration_native_thread()==native_b&&web_runtime_frame_get()==caught);
        web_runtime_frame_set(previous);free(caught);
        check(current(context)==KEY&&context->x[17]==UINT64_C(0xfeedfacecafebeef)&&context->v[7].d[1]==UINT64_C(0x0123456789abcdef));
        atomic_store(&stage,3);invoke(context,SWITCH,0);
        check(valid_varargs());
        check(context->tpidr==TP_A&&migration_native_thread()==native_a&&migration_get_tls()==11);
        check(current(context)==KEY&&context->x[17]==UINT64_C(0xfeedfacecafebeef));
    }
    for(unsigned i=0;i<8;i++)check(locals[i]==UINT64_C(0xabcdef0000000000)+depth*100+i);
}
void call_indirect(Ctx *context,uint64_t function){
    check(function==42);Frame *frame=calloc(1,sizeof *frame);if(!frame)abort();void *previous=web_runtime_frame_get();web_runtime_frame_set(frame);
    if(!setjmp(frame->boundary)){
        nested(context,8);
        guest_trap("expected migrated diagnostic",37);
    }else check(!strcmp(frame->error,"expected migrated diagnostic")&&frame->address==37&&web_runtime_frame_get()==frame);
    context->x[0]=0;atomic_store(&stage,5);web_runtime_frame_set(previous);free(frame);
}
int web_runtime_fiber_call(Ctx *context,void (*body)(Ctx *,void *),void *argument){
    Frame *frame=calloc(1,sizeof *frame);if(!frame)abort();void *previous=web_runtime_frame_get();web_runtime_frame_set(frame);
    if(!setjmp(frame->boundary))body(context,argument);else{fprintf(stderr,"unexpected fiber-loop diagnostic: %s\n",frame->error);check(0);}
    web_runtime_frame_set(previous);free(frame);return 0;
}
void web_runtime_frame_discard(void *frame){if(frame){check(frame!=web_runtime_frame_get());free(frame);}}
__attribute__((noinline)) static void publish(void){
    snprintf(report,sizeof report,"{\"ok\":%s,\"checks\":%u,\"failures\":%u,\"stage\":%u,\"guestTlsMigrated\":true,\"diagnosticFramePreserved\":true,\"suspendedFiberFinalized\":true,\"workerIds\":[%u,%u]}",
      atomic_load(&failures)?"false":"true",atomic_load(&checks),atomic_load(&failures),atomic_load(&stage),worker_a_id,worker_b_id);
    MAIN_THREAD_ASYNC_EM_ASM({Module['onHostFiberReport'](UTF8ToString($0));},report);
}
static void *worker_a(void *unused){
    (void)unused;worker_a_id=worker_id();native_a=migration_native_thread();migration_set_tls(11);
    Frame *frame=calloc(1,sizeof *frame);if(!frame)abort();web_runtime_frame_set(frame);Ctx context={.tpidr=TP_A};
    if(!setjmp(frame->boundary)){
        check(current(&context)==0);
        invoke(&context,SWITCH,KEY);check(atomic_load(&stage)==1&&current(&context)==0&&context.tpidr==TP_A&&web_runtime_frame_get()==frame);
        atomic_store(&stage,2);wait_stage(4);
        invoke(&context,SWITCH,KEY);check(atomic_load(&stage)==5&&current(&context)==0&&context.tpidr==TP_A&&web_runtime_frame_get()==frame);
        invoke(&context,FINALIZE,KEY);
        expect_error(&context,FINALIZE,KEY,"finalizing unknown or running fiber");
        expect_error(&context,SWITCH,KEY,"unknown or running fiber");
        // Reuse finalized slots without carrying the old suspended stack.
        for(unsigned i=0;i<4200;i++){
            context.x[0]=KEY;context.x[1]=42;context.x[2]=0x1234;context.x[3]=UINT64_C(0x7100100000);context.x[4]=65536;context.x[5]=i&1;
            if(!web_fiber_import(&context,INITIALIZE))abort();context.x[0]=KEY;if(!web_fiber_import(&context,FINALIZE))abort();
        }
        check(1);
        context.x[5]=2;expect_error(&context,INITIALIZE,KEY,"invalid fiber initialization");
    }else{check(0);fprintf(stderr,"host fiber fixture: %s\n",frame->error);}
    web_fiber_thread_cleanup();web_runtime_frame_set(NULL);free(frame);atomic_store(&stage,6);
    if(atomic_fetch_add(&finished,1)==1)publish();pthread_exit(NULL);
}
static void *worker_b(void *unused){
    (void)unused;worker_b_id=worker_id();native_b=migration_native_thread();migration_set_tls(22);
    Frame *frame=calloc(1,sizeof *frame);if(!frame)abort();web_runtime_frame_set(frame);Ctx context={.tpidr=TP_B};
    if(!setjmp(frame->boundary)){
        wait_stage(2);invoke(&context,SWITCH,KEY);
        check(atomic_load(&stage)==3&&current(&context)==0&&context.tpidr==TP_B&&web_runtime_frame_get()==frame);
    }else{check(0);fprintf(stderr,"host fiber fixture: %s\n",frame->error);}
    atomic_store(&stage,4);wait_stage(6);web_fiber_thread_cleanup();web_runtime_frame_set(NULL);free(frame);
    if(atomic_fetch_add(&finished,1)==1)publish();pthread_exit(NULL);
}
int host_fiber_start(void){
    atomic_store(&stage,0);atomic_store(&checks,0);atomic_store(&failures,0);atomic_store(&finished,0);
    Ctx context={0};context.x[0]=KEY;context.x[1]=42;context.x[2]=0x1234;context.x[3]=UINT64_C(0x7100100000);context.x[4]=1048576;context.x[5]=1;
    if(!web_fiber_import(&context,INITIALIZE))return 1;
    return pthread_create(&workers[0],NULL,worker_a,NULL)||pthread_create(&workers[1],NULL,worker_b,NULL);
}
int host_fiber_join(void){return pthread_join(workers[0],NULL)||pthread_join(workers[1],NULL);}
