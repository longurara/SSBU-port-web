/* Actual C pthread -> JS WebM/WebCodecs -> C bytes/completion integration.
 * Uses local media through WORKERFS, without running or changing the game. */
#include "runtime.h"
#include "host_movie_bridge.h"
#ifdef MOVIE_SDK_EXTRACTOR
#include "movie_sdk_probe_support.h"
#endif
#include <emscripten/emscripten.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_t driver;
static unsigned checks,failures,first_failure,limit,require_eof;
static char report[4096];
void guest_trap(const char *what,uint64_t address){fprintf(stderr,"movie bridge probe trap: %s %llx\n",what,(unsigned long long)address);abort();}
#define check(v) do{checks++;if(!(v)){failures++;if(!first_failure)first_failure=__LINE__;}}while(0)
static uint32_t u32(const void *p){uint32_t value;memcpy(&value,p,4);return value;}
static int64_t i64(const void *p){int64_t value;memcpy(&value,p,8);return value;}
static WebMovieReply request(unsigned op,uint32_t handle,int index,unsigned flags,int64_t time,void *data,size_t size){return web_movie_request(op,handle,index,flags,time,data,size);}
static void *run(void *unused){
    (void)unused;uint32_t extractor=0,decoder=0;uint8_t metadata[64]={0},sample[24]={0},output[40]={0};unsigned sdk=0,reader_calls=0;
    uint8_t *upload=NULL,*input=NULL,*pixels=NULL;FILE *file=NULL;unsigned fed=0,frames=0,eos=0;int eof=0,track=-1;
    struct {int64_t time;uint32_t hash,bytes,width,height;} observed[16]={0};
    WebMovieReply reply;
#ifdef MOVIE_SDK_EXTRACTOR
    check(movie_sdk_probe_begin()==0);sdk=1;reply=movie_sdk_probe_open(&extractor);check(reply.status==0&&reply.value>0);if(reply.status)goto finish;
#else
    reply=request(MOVIE_CREATE,0,0,0,0,NULL,0);check(reply.status==0);if(reply.status)goto finish;extractor=reply.value;
    file=fopen("/movie/input.webm","rb");check(file!=NULL);if(!file)goto finish;
    upload=malloc(1048576);check(upload!=NULL);if(!upload)goto finish;
    size_t n;while((n=fread(upload,1,1048576,file))>0){reply=request(MOVIE_APPEND,extractor,0,0,0,upload,n);check(reply.status==0);if(reply.status)goto finish;}
    check(!ferror(file));fclose(file);file=NULL;free(upload);upload=NULL;
    reply=request(MOVIE_OPEN,extractor,0,0,0,NULL,0);check(reply.status==0&&reply.value>0);if(reply.status)goto finish;
#endif
    unsigned tracks=reply.value;
    for(unsigned i=0;i<tracks;i++){
#ifdef MOVIE_SDK_EXTRACTOR
        reply=movie_sdk_probe_track((int)i,metadata);
#else
        reply=request(MOVIE_TRACK,extractor,(int)i,0,0,metadata,sizeof metadata);check(reply.status==0);if(reply.status)goto finish;
#endif
        check(reply.status==0);if(reply.status)goto finish;
        if(u32(metadata+4)==1&&u32(metadata+44)){track=(int)i;break;}
    }
    check(track>=0);if(track<0)goto finish;
    uint32_t maximum=u32(metadata+24),width=u32(metadata+8),height=u32(metadata+12);
    check(maximum>0&&maximum<=32u*1024u*1024u&&width>0&&width<=8192&&height>0&&height<=8192);if(!maximum||maximum>32u*1024u*1024u)goto finish;
    input=malloc(maximum);size_t pixel_capacity=(size_t)width*height+2*((width+1)/2)*((height+1)/2);pixels=malloc(pixel_capacity);check(input&&pixels);if(!input||!pixels)goto finish;
#ifdef MOVIE_SDK_EXTRACTOR
    reply=movie_sdk_probe_select(track);
#else
    reply=request(MOVIE_SELECT,extractor,track,1,0,NULL,0);
#endif
    check(reply.status==0);if(reply.status)goto finish;
    reply=request(MOVIE_CONFIGURE,extractor,track,17,0,NULL,0);check(reply.status==0&&reply.value);if(reply.status)goto finish;decoder=reply.value;
    while(fed<limit){
#ifdef MOVIE_SDK_EXTRACTOR
        reply=movie_sdk_probe_peek(sample);
#else
        reply=request(MOVIE_PEEK,extractor,0,0,0,sample,sizeof sample);
#endif
        if(reply.status==MOVIE_END){eof=1;break;}check(reply.status==0);if(reply.status)goto finish;
        uint32_t size=u32(sample+4);check(size>0&&size<=maximum);if(size>maximum)goto finish;
#ifdef MOVIE_SDK_EXTRACTOR
        reply=movie_sdk_probe_read(input,maximum);
#else
        reply=request(MOVIE_READ,extractor,0,0,0,input,maximum);
#endif
        check(reply.status==0);if(reply.status)goto finish;
        reply=request(MOVIE_SUBMIT,decoder,0,0,i64(sample+8),input,size);check(reply.status==0);if(reply.status)goto finish;
#ifdef MOVIE_SDK_EXTRACTOR
        reply=movie_sdk_probe_advance();
#else
        reply=request(MOVIE_ADVANCE,extractor,0,0,0,NULL,0);
#endif
        check(reply.status==0);if(reply.status)goto finish;fed++;
    }
    if(require_eof&&!eof){
#ifdef MOVIE_SDK_EXTRACTOR
        reply=movie_sdk_probe_peek(sample);
#else
        reply=request(MOVIE_PEEK,extractor,0,0,0,sample,sizeof sample);
#endif
        eof=reply.status==MOVIE_END;check(eof);
    }
    reply=request(MOVIE_EOS,decoder,0,0,0,NULL,0);check(reply.status==(eof?MOVIE_OK:MOVIE_INVALID));
    if(!eof){reply=request(MOVIE_FLUSH,decoder,0,0,0,NULL,0);check(reply.status==0);if(reply.status)goto finish;}
    /* Completed output owns its bytes even after its source is destroyed. */
#ifdef MOVIE_SDK_EXTRACTOR
    reply=movie_sdk_probe_close();
#else
    reply=request(MOVIE_CLOSE,extractor,0,0,0,NULL,0);
#endif
    check(reply.status==0);extractor=0;
    for(;;){
        reply=request(MOVIE_ACQUIRE,decoder,0,0,0,output,sizeof output);if(reply.status==MOVIE_NO_OUTPUT)break;check(reply.status==0);if(reply.status)goto finish;
        uint32_t slot=reply.value;
        if(u32(output+20)&4){check(eof&&slot==0);eos++;request(MOVIE_RELEASE,decoder,0,0,0,NULL,0);continue;}
        uint32_t size=u32(output+4);check(size==pixel_capacity&&slot>0&&frames<16);if(size>pixel_capacity||frames>=16)goto finish;
        reply=request(MOVIE_COPY,decoder,(int)slot,0,0,pixels,pixel_capacity);check(reply.status==0);if(reply.status)goto finish;
        uint32_t hash=2166136261u;for(size_t i=0;i<size;i++)hash=(hash^pixels[i])*16777619u;
        observed[frames].time=i64(output+8);observed[frames].hash=hash;observed[frames].bytes=size;observed[frames].width=u32(output+24);observed[frames].height=u32(output+28);frames++;
        reply=request(MOVIE_RELEASE,decoder,(int)slot,0,0,NULL,0);check(reply.status==0);
        reply=request(MOVIE_COPY,decoder,(int)slot,0,0,pixels,pixel_capacity);check(reply.status==MOVIE_INVALID);
    }
    check(frames==fed&&eos==(unsigned)eof);
finish:
    if(file)fclose(file);free(upload);free(input);free(pixels);
    if(decoder){reply=request(MOVIE_DECODER_CLOSE,decoder,0,0,0,NULL,0);check(reply.status==0);}
#ifdef MOVIE_SDK_EXTRACTOR
    if(sdk){reader_calls=movie_sdk_probe_reader_calls();check(movie_sdk_probe_finish()==0);}
#else
    if(extractor){reply=request(MOVIE_CLOSE,extractor,0,0,0,NULL,0);check(reply.status==0);}
#endif
    size_t pos=(size_t)snprintf(report,sizeof report,"{\"ok\":%s,\"checks\":%u,\"failures\":%u,\"firstFailureLine\":%u,\"fed\":%u,\"eof\":%s,\"eos\":%u,\"sdkExtractor\":%s,\"readerCalls\":%u,\"frames\":[",failures?"false":"true",checks,failures,first_failure,fed,eof?"true":"false",eos,sdk?"true":"false",reader_calls);
    for(unsigned i=0;i<frames;i++)pos+=(size_t)snprintf(report+pos,sizeof report-pos,"%s{\"timestamp\":%lld,\"hash\":\"%x\",\"bytes\":%u,\"width\":%u,\"height\":%u}",i?",":"",(long long)observed[i].time,observed[i].hash,observed[i].bytes,observed[i].width,observed[i].height);
    snprintf(report+pos,sizeof report-pos,"]}");
    MAIN_THREAD_ASYNC_EM_ASM({Module['onMovieProbeReport'](UTF8ToString($0));},report);return NULL;
}
EMSCRIPTEN_KEEPALIVE int movie_bridge_start(unsigned sample_limit,unsigned expect_eof){if(sample_limit<1||sample_limit>16)return 1;limit=sample_limit;require_eof=expect_eof;return pthread_create(&driver,NULL,run,NULL);}
EMSCRIPTEN_KEEPALIVE int movie_bridge_join(void){return pthread_join(driver,NULL);}
