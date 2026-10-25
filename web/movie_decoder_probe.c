/* Production SDK Extractor + Decoder + real OS events. Only the media container
 * is a finite test excerpt; every compressed byte and decoded pixel is real. */
#include "host_movie.h"
#include "movie_sdk_probe_support.h"
#include "host_event.h"
#include <emscripten/emscripten.h>
#include <emscripten/threading.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define BASE UINT64_C(0x8000000000)
#define DEC (BASE+0x1000)
#define BUF (BASE+0x1100)
#define PIX (BASE+0x1200)
#define FMT (BASE+0x1300)
#define OUT (BASE+0x1400)
#define EVENT_ARRAY (BASE+0x1500)
#define EVENT (BASE+0x1600)
int web_os_import(Ctx *,const char *);
int web_os_indirect(Ctx *,uint64_t);
static pthread_t driver;
static unsigned checks,failures,first_failure,limit;
static char report[65536];
static atomic_int race_phase,race_status;
GuestMemory *elf_memory(void);
EMSCRIPTEN_KEEPALIVE int movie_decoder_race_pending(void){return atomic_load(&race_phase);}
EMSCRIPTEN_KEEPALIVE void movie_decoder_race_entered(void){atomic_store(&race_phase,2);emscripten_futex_wake(&race_phase,1);}
EMSCRIPTEN_KEEPALIVE int movie_decoder_race_live(void){return (int)web_movie_live_decoders();}
static void *race_call(void *unused){
    (void)unused;web_memory_bind(elf_memory());Ctx c={0};c.x[0]=DEC;
    int handled=web_os_indirect(&c,rd64(rd64(DEC)+16*8));atomic_store(&race_status,handled?(int)c.x[0]:-1);web_memory_bind(NULL);return NULL;
}
void guest_trap(const char *what,uint64_t address){fprintf(stderr,"movie decoder probe trap: %s %llx\n",what,(unsigned long long)address);abort();}
#define check(v) do{checks++;if(!(v)){failures++;if(!first_failure)first_failure=__LINE__;}}while(0)
static int call(uint64_t object,unsigned slot,uint64_t a,uint64_t b,uint64_t p,uint64_t q,uint64_t r){
    Ctx c={0};c.x[0]=object;c.x[1]=a;c.x[2]=b;c.x[3]=p;c.x[4]=q;c.x[5]=r;c.x[19]=0x12345678;c.fn=0x7100001234;c.x[30]=0x7100005678;c.tpidr=0x7800010000;
    check(web_os_indirect(&c,rd64(rd64(object)+slot*8)));
    check(c.x[19]==0x12345678&&c.fn==0x7100001234&&c.x[30]==0x7100005678&&c.tpidr==0x7800010000);return (int)c.x[0];
}
static void value(const char *name,uint64_t object,uint64_t a,uint64_t b){Ctx c={0};c.x[0]=object;c.x[1]=a;c.x[2]=b;check(web_movie_import(&c,name));}
static uint64_t os(const char *name,uint64_t key,uint64_t a,uint64_t b){Ctx c={0};c.x[0]=key;c.x[1]=a;c.x[2]=b;check(web_os_import(&c,name));return c.x[0];}
static uint32_t u32(const void *p){uint32_t v;memcpy(&v,p,4);return v;}
static uint32_t pixel_hash(uint64_t address,uint32_t size){uint32_t hash=2166136261u;uint8_t bytes[4096];for(uint32_t at=0;at<size;){uint32_t n=size-at>sizeof bytes?sizeof bytes:size-at;web_read(address+at,bytes,n);for(uint32_t i=0;i<n;i++)hash=(hash^bytes[i])*16777619u;at+=n;}return hash;}
static void *run(void *unused){
    (void)unused;unsigned begun=0,constructed=0,events=0,fed=0,frames=0,eos=0,eof=0,held_count=0,max_held=0,waits=0,raced=0;uint32_t source=0,owner=0,width=0,height=0,pixel_size=0;uint64_t pixels=0;
    struct {uint64_t time;uint32_t hash,bytes;} observed[256]={0};unsigned held[48];uint8_t metadata[64];
    check(movie_sdk_probe_begin()==0);begun=1;
    WebMovieReply reply=movie_sdk_probe_open(&source);check(!reply.status&&reply.value);if(reply.status)goto finish;
    int track=-1;for(unsigned i=0;i<reply.value;i++){WebMovieReply t=movie_sdk_probe_track((int)i,metadata);check(!t.status);if(t.status)goto finish;if(u32(metadata+4)==1&&u32(metadata+44)){track=(int)i;break;}}
    check(track>=0);if(track<0)goto finish;width=u32(metadata+8);height=u32(metadata+12);pixel_size=width*height+2*((width+1)/2)*((height+1)/2);
    reply=movie_sdk_probe_select(track);check(!reply.status);if(reply.status)goto finish;
    Ctx c={0};c.x[0]=DEC;c.x[1]=u32(metadata);c.x[2]=4;c.x[3]=1;c.x[4]=1;
    check(web_os_import(&c,"_ZN5movie7DecoderC1ENS_11DecoderTypeENS_19DecoderOutputFormatENS_11DecoderModeEi"));constructed=1;owner=(uint32_t)rd64(DEC+8);
    value("_ZN5movie6BufferC1Em",BUF,4096,0); // GetInput must release this owned allocation.
    value("_ZN5movie6BufferC1Ev",PIX,0,0);value("_ZN5movie9MediaDataC1Ev",FMT,0,0);
    check(call(DEC,5,0,0,0,0,0)==MOVIE_INVALID);
    int configured=call(DEC,2,movie_sdk_probe_metadata(),0,0,0,0);check(configured==0);if(configured)goto finish;
    for(unsigned i=0;i<4;i++){os("nnosInitializeEvent",EVENT+i*32,0,1);wr64(EVENT_ARRAY+i*8,EVENT+i*32);}events=1;
    // Events registered after Start must receive both existing input readiness
    // and the known output format, using actual HostEvent synchronization.
    check(call(DEC,5,0,0,0,0,0)==0);check(call(DEC,3,EVENT_ARRAY,0,0,0,0)==0);
    check(os("_ZN2nn2os12TryWaitEventEPNS0_9EventTypeE",EVENT,0,0)==1);
    check(os("_ZN2nn2os12TryWaitEventEPNS0_9EventTypeE",EVENT+64,0,0)==1);
    check(call(DEC,4,FMT,0,0,0,0)==0);uint64_t format_width=0;check(web_movie_metadata_get(FMT,"width",WEB_MOVIE_I32,&format_width)&&format_width==width);
    check(call(DEC,14,OUT,OUT+4,0,0,0)==0&&rd32(OUT)==pixel_size&&rd32(OUT+4)==1);
    pixels=web_movie_guest_alloc(&c,pixel_size);check(pixels!=0);if(!pixels)goto finish;value("_ZN5movie6BufferC1EPvm",PIX,pixels,pixel_size);
    while(!eos){
        unsigned progress=0;
        while(!eof){
            int status=call(DEC,9,OUT,OUT+4,0,0,0);if(status==MOVIE_NO_INPUT)break;check(status==0);if(status)goto finish;unsigned input=rd32(OUT);check(input<8);
            status=call(movie_sdk_probe_extractor(),9,OUT+8,0,0,0,0);
            if(status==MOVIE_END){eof=1;check(fed==limit);check(call(DEC,11,input,0,0,0,4)==0);progress=1;break;}
            check(status==0&&fed<limit);if(status||fed>=limit)goto finish;
            check(call(DEC,10,input,BUF,0,0,0)==0);check(!rd8(BUF+16)&&rd64(BUF+40)&&rd64(BUF+24)>=u32(metadata+24));
            check(call(movie_sdk_probe_extractor(),10,BUF,0,0,0,0)==0);
            check(call(movie_sdk_probe_extractor(),12,OUT+16,0,0,0,0)==0);
            check(call(DEC,11,input,UINT32_MAX,rd32(BUF+4),rd64(OUT+16),0)==MOVIE_INVALID);
            status=call(DEC,11,input,rd32(BUF),rd32(BUF+4),rd64(OUT+16),0);check(status==0);if(status)goto finish;
            check(call(movie_sdk_probe_extractor(),11,0,0,0,0,0)==0);fed++;progress=1;
        }
        for(;;){
            int status=call(DEC,12,OUT,OUT+8,OUT+16,OUT+20,0);if(status==MOVIE_NO_OUTPUT)break;check(status==0);if(status)goto finish;
            unsigned index=rd32(OUT),flags=rd32(OUT+16);check(index<48&&held_count<48);
            if(flags&4){eos++;check(eof);check(call(DEC,13,index,PIX,0,0,0)==0&&rd32(PIX+4)==0);check(call(DEC,15,index,0,0,0,0)==0);break;}
            check(frames<256);if(frames>=256)goto finish;
            wr64(PIX+24,1);check(call(DEC,13,index,PIX,0,0,0)==MOVIE_INVALID);wr64(PIX+24,pixel_size);
            check(call(DEC,17,index,PIX,FMT,0,0)==0);check(rd32(PIX)==0&&rd32(PIX+4)==pixel_size&&rd64(PIX+32)==rd64(OUT+8));
            check(call(DEC,18,index,FMT,0,0,0)==0);
            uint64_t uv=0;check(web_movie_metadata_get(FMT,"nv12-uv-offset",WEB_MOVIE_I32,&uv)&&uv==(uint64_t)width*height);
            observed[frames].time=rd64(PIX+32);observed[frames].bytes=rd32(PIX+4);observed[frames].hash=pixel_hash(pixels,pixel_size);frames++;
            held[held_count++]=index;if(held_count>max_held)max_held=held_count;progress=1;
        }
        if(held_count==48||eof){unsigned n=eof?held_count:1;for(unsigned i=0;i<n;i++){unsigned index=held[0];memmove(held,held+1,(--held_count)*sizeof *held);check(call(DEC,15,index,0,0,0,0)==0);check(call(DEC,13,index,PIX,0,0,0)==MOVIE_INVALID);}if(n)progress=1;}
        check(call(DEC,16,0,0,0,0,0)==0);
        if(!progress&&!eos){os("_ZN2nn2os14TimedWaitEventEPNS0_9EventTypeENS_8TimeSpanE",EVENT+32,10000000,0);if(++waits>3000){check(0);goto finish;}}
    }
    check(frames==fed&&eos==1&&held_count==0);check(call(DEC,12,OUT,OUT+8,OUT+16,OUT+20,0)==MOVIE_NO_OUTPUT);
    check(call(DEC,8,0,0,0,0,0)==0);check(call(DEC,9,OUT,OUT+4,0,0,0)==MOVIE_NO_INPUT);
    check(call(DEC,5,0,0,0,0,0)==0);check(call(DEC,9,OUT,OUT+4,0,0,0)==0);check(call(DEC,6,0,0,0,0,0)==0);
    check(call(DEC,3,0,0,0,0,0)==0);
    pthread_t racing;atomic_store(&race_status,-2);atomic_store(&race_phase,1);
    int created=pthread_create(&racing,NULL,race_call,NULL);check(created==0);
    if(!created){
        while(atomic_load(&race_phase)==1)emscripten_futex_wait(&race_phase,1,1000);
        call(DEC,0,0,0,0,0,0);constructed=0;check(pthread_join(racing,NULL)==0);check(atomic_load(&race_status)==0);check(web_movie_live_decoders()==0);raced=1;
    }
finish:
    if(constructed){check(call(DEC,3,0,0,0,0,0)==0);call(DEC,0,0,0,0,0,0);check(web_movie_live_decoders()==0);}
    if(events){for(unsigned i=0;i<4;i++)os("_ZN2nn2os10ClearEventEPNS0_9EventTypeE",EVENT+i*32,0,0);web_movie_notify(owner,15);for(unsigned i=0;i<4;i++){check(os("_ZN2nn2os12TryWaitEventEPNS0_9EventTypeE",EVENT+i*32,0,0)==0);os("nnosFinalizeEvent",EVENT+i*32,0,0);}check(web_os_try_signal_event(EVENT)==0);}
    unsigned reader_calls=movie_sdk_probe_reader_calls();
    if(begun){value("_ZN5movie6BufferD1Ev",BUF,0,0);value("_ZN5movie6BufferD1Ev",PIX,0,0);value("_ZN5movie9MediaDataD1Ev",FMT,0,0);if(pixels)web_movie_guest_free(&(Ctx){0},pixels);check(movie_sdk_probe_finish()==0);}
    size_t pos=(size_t)snprintf(report,sizeof report,"{\"ok\":%s,\"checks\":%u,\"failures\":%u,\"firstFailureLine\":%u,\"fed\":%u,\"eof\":%s,\"eos\":%u,\"sdkExtractor\":true,\"sdkDecoder\":true,\"concurrentDestroy\":%s,\"readerCalls\":%u,\"maxHeldOutputs\":%u,\"waits\":%u,\"frames\":[",failures?"false":"true",checks,failures,first_failure,fed,eof?"true":"false",eos,raced?"true":"false",reader_calls,max_held,waits);
    for(unsigned i=0;i<frames;i++)pos+=(size_t)snprintf(report+pos,sizeof report-pos,"%s{\"timestamp\":%llu,\"hash\":\"%x\",\"bytes\":%u,\"width\":%u,\"height\":%u}",i?",":"",(unsigned long long)observed[i].time,observed[i].hash,observed[i].bytes,width,height);
    snprintf(report+pos,sizeof report-pos,"]}");MAIN_THREAD_ASYNC_EM_ASM({Module['onMovieProbeReport'](UTF8ToString($0));},report);return NULL;
}
EMSCRIPTEN_KEEPALIVE int movie_bridge_start(unsigned count,unsigned expect_eof){if(!expect_eof||count<1||count>256)return 1;limit=count;return pthread_create(&driver,NULL,run,NULL);}
EMSCRIPTEN_KEEPALIVE int movie_bridge_join(void){return pthread_join(driver,NULL);}
