#include "runtime.h"
#include "host_movie_bridge.h"
#include <emscripten/emscripten.h>
#include <pthread.h>

typedef struct {pthread_mutex_t lock;pthread_cond_t changed;int done,status;uint32_t value;} MovieRequest;
EMSCRIPTEN_KEEPALIVE void movie_complete(uintptr_t token,int status,uint32_t value){
    MovieRequest *request=(MovieRequest *)token;
    pthread_mutex_lock(&request->lock);request->status=status;request->value=value;request->done=1;
    pthread_cond_signal(&request->changed);pthread_mutex_unlock(&request->lock);
}
WebMovieReply web_movie_request(unsigned operation,uint32_t handle,int index,unsigned flags,int64_t time,void *staging,size_t length){
    if(length>32u*1024u*1024u||time>INT64_C(9007199254740991)||time<-INT64_C(9007199254740991))return (WebMovieReply){MOVIE_INVALID,0};
    MovieRequest request={PTHREAD_MUTEX_INITIALIZER,PTHREAD_COND_INITIALIZER,0,MOVIE_FAILED,0};
    MAIN_THREAD_ASYNC_EM_ASM({
        const request=({token:$0>>>0,operation:$1,handle:$2>>>0,index:$3,flags:$4>>>0,time:$5,pointer:$6>>>0,length:$7});
        if(typeof Module['webmovieDispatch']==='function')Module['webmovieDispatch'](request);
        else Module['_movie_complete'](request.token,0x1000,0);
    },(uintptr_t)&request,operation,handle,index,flags,(double)time,(uintptr_t)staging,length);
    pthread_mutex_lock(&request.lock);while(!request.done)pthread_cond_wait(&request.changed,&request.lock);pthread_mutex_unlock(&request.lock);
    WebMovieReply reply={request.status,request.value};pthread_cond_destroy(&request.changed);pthread_mutex_destroy(&request.lock);return reply;
}
