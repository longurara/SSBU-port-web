#include "runtime.h"
#include "host_gpu_bridge.h"
#include <emscripten/emscripten.h>
#include <pthread.h>

typedef struct {pthread_mutex_t lock;pthread_cond_t changed;int done,status;uint32_t value;} GpuRequest;

void gpu_complete(uintptr_t pointer,int status,uint32_t value) {
    GpuRequest *request=(GpuRequest *)pointer;
    pthread_mutex_lock(&request->lock);
    request->status=status;request->value=value;request->done=1;
    pthread_cond_signal(&request->changed);pthread_mutex_unlock(&request->lock);
}

WebGpuReply web_gpu_request(unsigned operation,uint32_t handle,uint64_t offset,uint64_t size,void *staging,size_t length) {
    if(offset>0x7fffffff || size>0x7fffffff || length>128*1024*1024)guest_trap("GPU resource request exceeds bridge limits",offset);
    GpuRequest request={PTHREAD_MUTEX_INITIALIZER,PTHREAD_COND_INITIALIZER,0,0,0};
    MAIN_THREAD_ASYNC_EM_ASM({
        Module['webgpuDispatch']({token:$0>>>0,operation:$1,handle:$2>>>0,offset:$3,size:$4,pointer:$5>>>0,length:$6});
    }, (uintptr_t)&request,operation,handle,(uint32_t)offset,(uint32_t)size,(uintptr_t)staging,length);
    pthread_mutex_lock(&request.lock);
    while(!request.done)pthread_cond_wait(&request.changed,&request.lock);
    pthread_mutex_unlock(&request.lock);
    WebGpuReply reply={request.status,request.value};
    pthread_cond_destroy(&request.changed);pthread_mutex_destroy(&request.lock);
    return reply;
}
