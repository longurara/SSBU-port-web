/* Stackful FiberType adapter for the static Wasm module. Guest addresses stay
 * virtual; guest registers and diagnostic frames follow the suspended stack. */
#include "host_fiber.h"
#include "host_fiber_tls.h"
#include "portable_fiber.h"
#include "runtime_frame.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>

GuestMemory *elf_memory(void);
int web_runtime_fiber_call(Ctx *,void (*body)(Ctx *,void *),void *);
#define FIBER_CAPACITY 4096
#define C_STACK_SIZE (1024*1024)
#define ASYNC_STACK_SIZE (1024*1024)
typedef struct HostFiber {
    uint64_t key,entry,arg,stack,size,tp,flags;
    int used;
    atomic_int running;
    emscripten_fiber_t fiber;
    void *c_stack,*async_stack,*frame;
    Ctx context;
} HostFiber;
typedef struct {
    emscripten_fiber_t root;
    HostFiber *current;
    void *async_stack,*frame;
    uint64_t tp;
} FiberThread;
static HostFiber fibers[FIBER_CAPACITY];
static pthread_mutex_t registry=PTHREAD_MUTEX_INITIALIZER;
static void fiber_entry(void *argument);
static FiberThread *thread_state(void){
    FiberThread *state=web_fiber_thread_state();
    if(!state){
        state=calloc(1,sizeof *state);
        if(!state)guest_trap("fiber thread allocation failed",0);
        state->async_stack=aligned_alloc(16,ASYNC_STACK_SIZE);
        if(!state->async_stack){free(state);guest_trap("fiber root stack allocation failed",0);}
        emscripten_fiber_init_from_current_context(&state->root,state->async_stack,ASYNC_STACK_SIZE);
        web_fiber_set_thread_state(state);
    }
    return state;
}
/* Caller holds registry. Records are never moved; finalization requires no
 * executing or resuming owner and no retained frame on an active stack. */
static HostFiber *find(uint64_t key){
    for(unsigned i=0;i<FIBER_CAPACITY;i++)if(fibers[i].used&&fibers[i].key==key)return &fibers[i];
    return NULL;
}
void web_fiber_thread_cleanup(void){
    FiberThread *thread=web_fiber_thread_state();if(!thread)return;
    if(thread->current)guest_trap("exiting thread from active guest fiber",thread->current->key);
    web_fiber_set_thread_state(NULL);free(thread->async_stack);free(thread);
}
static void switch_to(Ctx *context,uint64_t key){
    FiberThread *thread=thread_state();HostFiber *self=thread->current,*next=NULL;
    if(self&&self->key==key)guest_trap("switch to current fiber",key);
    if(!self&&!key)guest_trap("switch from root to root fiber",0);
    pthread_mutex_lock(&registry);
    if(key){
        next=find(key);
        if(!next||atomic_load(&next->running)){pthread_mutex_unlock(&registry);guest_trap("unknown or running fiber",key);}
        if(!next->c_stack){
            // Wasm32 malloc guarantees 8-byte alignment; the C stack ABI and
            // EM_ASM argument buffers require 16. fiber_init does not round it.
            void *c_stack=aligned_alloc(16,C_STACK_SIZE),*async_stack=aligned_alloc(16,ASYNC_STACK_SIZE);
            if(!c_stack||!async_stack){free(c_stack);free(async_stack);pthread_mutex_unlock(&registry);guest_trap("fiber stacks allocation failed",key);}
            next->c_stack=c_stack;next->async_stack=async_stack;
            emscripten_fiber_init(&next->fiber,fiber_entry,next,c_stack,C_STACK_SIZE,async_stack,ASYNC_STACK_SIZE);
        }
        atomic_store(&next->running,1);next->tp=context->tpidr;
    }
    pthread_mutex_unlock(&registry);
    void *frame=web_runtime_frame_get();
    if(self)self->frame=frame;else{thread->frame=frame;thread->tp=context->tpidr;}
    thread->current=next;
    web_runtime_frame_set(next?next->frame:thread->frame);
    emscripten_fiber_t *from=self?&self->fiber:&thread->root,*to=next?&next->fiber:&thread->root;
    if(!web_portable_fiber_swap_tracked(from,to,self?&self->running:NULL)){
        thread->current=self;web_runtime_frame_set(frame);
        if(next)atomic_store(&next->running,0);
        guest_trap("static fiber migration adapter rejected",key);
    }
    // The saved 'thread' pointer belongs to the source pthread. Reacquire the
    // destination's state after migration, and restore the guest thread block.
    FiberThread *destination=thread_state();
    context->tpidr=self?self->tp:destination->tp;
}
static void fiber_loop(Ctx *context,void *argument){
    HostFiber *fiber=argument;
    for(;;){
        context->tpidr=fiber->tp;context->x[0]=fiber->arg;context->x[30]=UINT64_C(0x7300000000);
        call_indirect(context,fiber->entry);
        switch_to(context,context->x[0]);
    }
}
static void fiber_entry(void *argument){
    HostFiber *fiber=argument;Ctx *context=&fiber->context;
    memset(context,0,sizeof *context);context->x[31]=(fiber->stack+fiber->size)&~UINT64_C(15);
    web_runtime_fiber_call(context,fiber_loop,fiber);
    // protected_call has already sent a terminal diagnostic to the owner.
    // Leave the worker available for termination, without resuming guest code.
    emscripten_exit_with_live_runtime();
}
int web_fiber_import(Ctx *context,const char *name){
    if(!strcmp(name,"_ZN2nn2os15InitializeFiberEPNS0_9FiberTypeEPFS2_PvES3_S3_mi")){
        uint64_t key=context->x[0],stack=context->x[3],size=context->x[4];
        // SDK bit 0 skips its optional stack-alias allocation. Our guest stack
        // uses the supplied virtual range (also the SDK's allocation-fallback
        // path); saving all virtual registers does not depend on this flag.
        if(!key||!context->x[1]||!stack||!size||stack+size<stack||(stack&15)||(size&15)||(context->x[5]&~UINT64_C(1)))guest_trap("invalid fiber initialization",key);
        pthread_mutex_lock(&registry);HostFiber *fiber=find(key);
        if(fiber){pthread_mutex_unlock(&registry);guest_trap("fiber already initialized",key);}
        for(unsigned i=0;i<FIBER_CAPACITY;i++)if(!fibers[i].used){fiber=&fibers[i];break;}
        if(!fiber){pthread_mutex_unlock(&registry);guest_trap("fiber registry capacity",key);}
        memset(fiber,0,sizeof *fiber);fiber->key=key;fiber->entry=context->x[1];fiber->arg=context->x[2];fiber->stack=stack;fiber->size=size;fiber->flags=context->x[5];fiber->used=1;
        pthread_mutex_unlock(&registry);context->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn2os13SwitchToFiberEPNS0_9FiberTypeE")){switch_to(context,context->x[0]);return 1;}
    if(!strcmp(name,"_ZN2nn2os15GetCurrentFiberEv")){
        FiberThread *thread=web_fiber_thread_state();context->x[0]=thread&&thread->current?thread->current->key:0;return 1;
    }
    if(!strcmp(name,"_ZN2nn2os13FinalizeFiberEPNS0_9FiberTypeE")){
        uint64_t key=context->x[0];pthread_mutex_lock(&registry);HostFiber *fiber=find(key);
        if(!fiber||atomic_load(&fiber->running)){pthread_mutex_unlock(&registry);guest_trap("finalizing unknown or running fiber",key);}
        web_runtime_frame_discard(fiber->frame);
        free(fiber->c_stack);free(fiber->async_stack);memset(fiber,0,sizeof *fiber);
        pthread_mutex_unlock(&registry);return 1;
    }
    return 0;
}
