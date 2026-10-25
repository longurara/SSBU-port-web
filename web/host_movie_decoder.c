/* SDK Decoder virtual calls over the real WebCodecs backend. All buffers
 * exposed to the game use its allocator and remain guest virtual addresses. */
#include "host_movie.h"
#include "host_movie_bridge.h"
#include "host_event.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <emscripten/emscripten.h>

#define DECODER_VTABLE UINT64_C(0x7800000c00)
#define DECODER_FUNCTION UINT64_C(0x7500004000)
#define GAME_DELETE UINT64_C(0x710392e590)
#define BYTE_LIMIT (32u*1024u*1024u)
typedef struct {unsigned used;uint32_t backend,size,flags,width,height,stride,uv;int64_t time;} Output;
typedef struct Decoder {
    uint64_t key,input[8];uint32_t owner,backend,capacity,width,height,type,format,mode,rate,channels,output_count;
    unsigned offered,client,pending,references;int started,flushed,eos,last_error;
    Output output[64];atomic_int closing;pthread_mutex_t gate;struct Decoder *next;
} Decoder;
static Decoder *decoders;
static pthread_mutex_t decoder_lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t decoder_changed=PTHREAD_COND_INITIALIZER;
/* This mutex is never held across a JS request, guest callback or decoder gate
 * acquisition. A JS notification therefore cannot wait on its own completion.
 * Register/unregister returning also proves all previous event signals ended. */
static pthread_mutex_t notify_lock=PTHREAD_MUTEX_INITIALIZER;
static struct {uint32_t owner;unsigned pending;uint64_t events[4];} notifications[64];
static uint32_t next_owner=1;
static uint32_t u32(const void *p){uint32_t v;memcpy(&v,p,4);return v;}
static int64_t i64(const void *p){int64_t v;memcpy(&v,p,8);return v;}
EMSCRIPTEN_KEEPALIVE void web_movie_notify(uint32_t owner,unsigned mask){
    if(!owner)return;pthread_mutex_lock(&notify_lock);
    for(unsigned i=0;i<64;i++)if(notifications[i].owner==owner){
        notifications[i].pending|=mask&15;
        for(unsigned j=0;j<4;j++)if((mask&(1u<<j))&&notifications[i].events[j])web_os_try_signal_event(notifications[i].events[j]);
        break;
    }
    pthread_mutex_unlock(&notify_lock);
}
static uint32_t notifier_create(void){
    uint32_t owner=0;pthread_mutex_lock(&notify_lock);
    if(next_owner<=INT32_MAX)for(unsigned i=0;i<64;i++)if(!notifications[i].owner){owner=next_owner++;notifications[i].owner=owner;break;}
    pthread_mutex_unlock(&notify_lock);return owner;
}
static void notifier_clear(uint32_t owner,unsigned mask){
    if(!owner)return;
    pthread_mutex_lock(&notify_lock);for(unsigned i=0;i<64;i++)if(notifications[i].owner==owner){notifications[i].pending&=~mask;break;}pthread_mutex_unlock(&notify_lock);
}
static void notifier_register(uint32_t owner,const uint64_t events[4]){
    if(!owner)return;
    pthread_mutex_lock(&notify_lock);
    for(unsigned i=0;i<64;i++)if(notifications[i].owner==owner){
        memcpy(notifications[i].events,events,sizeof notifications[i].events);
        for(unsigned j=0;j<4;j++)if(events[j]&&(notifications[i].pending&(1u<<j)))web_os_try_signal_event(events[j]);break;
    }
    pthread_mutex_unlock(&notify_lock);
}
static void notifier_destroy(uint32_t owner){
    if(!owner)return;
    pthread_mutex_lock(&notify_lock);for(unsigned i=0;i<64;i++)if(notifications[i].owner==owner){memset(&notifications[i],0,sizeof notifications[i]);break;}pthread_mutex_unlock(&notify_lock);
}
static WebMovieReply request(Decoder *d,unsigned op,int index,int64_t time,void *data,size_t size){return web_movie_request(op,d->backend,index,0,time,data,size);}
static void decoder_release(Decoder *d){
    pthread_mutex_unlock(&d->gate);pthread_mutex_lock(&decoder_lock);d->references--;pthread_cond_broadcast(&decoder_changed);pthread_mutex_unlock(&decoder_lock);
}
static Decoder *decoder_acquire(uint64_t key){
    pthread_mutex_lock(&decoder_lock);Decoder *d=decoders;while(d&&(d->key!=key||atomic_load(&d->closing)))d=d->next;
    if(d)d->references++;pthread_mutex_unlock(&decoder_lock);
    if(d){pthread_mutex_lock(&d->gate);if(atomic_load(&d->closing)){decoder_release(d);return NULL;}}return d;
}
static void free_inputs(Ctx *c,Decoder *d){for(unsigned i=0;i<8;i++){if(d->input[i])web_movie_guest_free(c,d->input[i]);d->input[i]=0;}d->capacity=0;}
static void decoder_destroy(Ctx *c,uint64_t key,int deleting){
    pthread_mutex_lock(&decoder_lock);Decoder **link=&decoders;while(*link&&(*link)->key!=key)link=&(*link)->next;Decoder *d=*link;
    if(d){atomic_store(&d->closing,1);*link=d->next;while(d->references)pthread_cond_wait(&decoder_changed,&decoder_lock);}pthread_mutex_unlock(&decoder_lock);
    if(d){if(d->backend)request(d,MOVIE_DECODER_CLOSE,0,0,NULL,0);notifier_destroy(d->owner);free_inputs(c,d);pthread_mutex_destroy(&d->gate);free(d);}
    wr64(key+8,0);if(deleting)web_movie_guest_call(c,GAME_DELETE,key,0,0,0,0);
}
static void decoder_construct(Ctx *c){
    Decoder *d=calloc(1,sizeof *d);if(!d)guest_trap("movie Decoder allocation failed",c->x[0]);
    d->key=c->x[0];d->type=(uint32_t)c->x[1];d->format=(uint32_t)c->x[2];d->mode=(uint32_t)c->x[3];d->owner=notifier_create();
    d->output_count=d->type==5?64:48;
    pthread_mutex_init(&d->gate,NULL);atomic_init(&d->closing,0);
    pthread_mutex_lock(&decoder_lock);d->next=decoders;decoders=d;pthread_mutex_unlock(&decoder_lock);
    for(unsigned i=0;i<64;i++)wr64(DECODER_VTABLE+i*8,DECODER_FUNCTION+i*16);wr64(d->key,DECODER_VTABLE);wr64(d->key+8,d->owner);
}
static int configure(Ctx *c,Decoder *d,uint64_t md){
    uint64_t source=0,track=0,capacity=0;if(!md||d->started)return MOVIE_INVALID;
    if(!d->owner)return MOVIE_FAILED;
    int audio=d->type==5;
    if((!audio&&d->type!=2&&d->type!=3)||d->format!=(audio?1u:4u)||d->mode!=1)return MOVIE_UNSUPPORTED;
    if(!web_movie_metadata_get(md,"nn-extractor",WEB_MOVIE_POINTER,&source)||!source||source>INT32_MAX||!web_movie_metadata_get(md,"nn-track",WEB_MOVIE_I32,&track)||track>INT32_MAX)return MOVIE_INVALID;
    uint8_t info[64];WebMovieReply reply=web_movie_request(MOVIE_TRACK,(uint32_t)source,(int)track,0,0,info,sizeof info);if(reply.status)return reply.status;
    uint32_t w=u32(info+8),h=u32(info+12),rate=u32(info+16),channels=u32(info+20),minimum=u32(info+24);
    if(u32(info)!=d->type||!u32(info+44))return MOVIE_UNSUPPORTED;
    uint64_t pixels=(uint64_t)w*h+2*((w+1)/2)*((h+1)/2);
    if(audio){if(rate<8000||rate>192000||channels<1||channels>8)return MOVIE_UNSUPPORTED;}
    else if(!w||!h||w>8192||h>8192||pixels>BYTE_LIMIT)return MOVIE_UNSUPPORTED;
    if(!web_movie_metadata_get(md,"max-input-size",WEB_MOVIE_I32,&capacity))capacity=minimum;
    if(capacity<minimum||!capacity||capacity>BYTE_LIMIT)return MOVIE_INVALID;
    uint64_t input[8]={0};for(unsigned i=0;i<8;i++)if(!(input[i]=web_movie_guest_alloc(c,capacity))){for(unsigned j=0;j<i;j++)web_movie_guest_free(c,input[j]);return MOVIE_FAILED;}
    reply=web_movie_request(MOVIE_CONFIGURE,(uint32_t)source,(int)track,d->owner,8,NULL,0);
    if(reply.status){for(unsigned i=0;i<8;i++)web_movie_guest_free(c,input[i]);return reply.status;}
    if(d->backend)request(d,MOVIE_DECODER_CLOSE,0,0,NULL,0);free_inputs(c,d);
    d->backend=reply.value;memcpy(d->input,input,sizeof input);d->capacity=(uint32_t)capacity;d->width=w;d->height=h;d->rate=rate;d->channels=channels;
    d->offered=d->client=d->pending=0;d->flushed=0;d->eos=0;d->last_error=0;memset(d->output,0,sizeof d->output);notifier_clear(d->owner,15);return MOVIE_OK;
}
static int fill_format(Ctx *c,Decoder *d,uint64_t md,const Output *out){
    if(!md||!d->backend)return MOVIE_INVALID;uint32_t w=out&&out->width?out->width:d->width,h=out&&out->height?out->height:d->height;
    if(d->type==5){
        web_movie_metadata_clear(c,md);int ok=web_movie_metadata_string(c,md,"mime","audio/raw");
        ok&=web_movie_metadata_set(c,md,"sample-rate",WEB_MOVIE_I32,d->rate);
        ok&=web_movie_metadata_set(c,md,"channel-count",WEB_MOVIE_I32,d->channels);
        ok&=web_movie_metadata_set(c,md,"pcm-encoding",WEB_MOVIE_I32,2);return ok?MOVIE_OK:MOVIE_FAILED;
    }
    web_movie_metadata_clear(c,md);int ok=web_movie_metadata_string(c,md,"mime","video/raw");
    const struct {const char *key;uint32_t value;} fields[]={
        {"width",w},{"height",h},{"stride",w},{"slice-height",h},{"crop-width",w},{"crop-height",h},{"color-format",21},
        {"nv12-width",w},{"nv12-height",h},{"nv12-y-stride",out&&out->stride?out->stride:w},{"nv12-uv-stride",2*((w+1)/2)},
        {"nv12-y-offset",0},{"nv12-uv-offset",out&&out->uv?out->uv:w*h},{"nv12-colorspace",0}};
    for(unsigned i=0;i<sizeof fields/sizeof fields[0];i++)ok&=web_movie_metadata_set(c,md,fields[i].key,WEB_MOVIE_I32,fields[i].value);return ok?MOVIE_OK:MOVIE_FAILED;
}
static int halt(Decoder *d,int stop){
    if(!d->backend)return MOVIE_INVALID;int status=request(d,MOVIE_RESET,0,0,NULL,0).status;if(status)return status;
    d->offered=d->client=d->pending=0;d->flushed=1;if(stop)d->started=0;d->eos=0;d->last_error=0;memset(d->output,0,sizeof d->output);notifier_clear(d->owner,15);return MOVIE_OK;
}
static int acquire_input(Decoder *d,uint64_t index,uint64_t more){
    if(!index||!more)return MOVIE_INVALID;wr32(index,UINT32_MAX);wr32(more,0);
    if(!d->backend||!d->started||d->flushed||d->eos)return MOVIE_NO_INPUT;
    WebMovieReply reply=request(d,MOVIE_INPUT_AVAILABLE,0,0,NULL,0);if(reply.status)return reply.status;
    unsigned returned=reply.value&d->pending;d->pending&=~returned;d->offered|=returned;
    if(!d->offered){notifier_clear(d->owner,1);return MOVIE_NO_INPUT;}
    unsigned slot=(unsigned)__builtin_ctz(d->offered);d->offered&=~(1u<<slot);d->client|=1u<<slot;wr32(index,slot);wr32(more,(uint32_t)__builtin_popcount(d->offered));return MOVIE_OK;
}
static int input_buffer(Ctx *c,Decoder *d,int index,uint64_t out){
    if(!out||index<0||index>=8||!(d->client&(1u<<index))||!d->input[index])return MOVIE_INVALID;
    Ctx value=*c;value.x[0]=out;web_movie_import(&value,"_ZN5movie6BufferD1Ev");
    value=*c;value.x[0]=out;value.x[1]=d->input[index];value.x[2]=d->capacity;web_movie_import(&value,"_ZN5movie6BufferC1EPvm");return MOVIE_OK;
}
static int send_input(Decoder *d,int index,int offset,int size,int64_t time,unsigned flags){
    if(!d->backend||!d->started||d->flushed||d->eos||index<0||index>=8||!(d->client&(1u<<index))||offset<0||size<0||(uint64_t)offset+(uint32_t)size>d->capacity)return MOVIE_INVALID;
    if(flags&~5u)return MOVIE_UNSUPPORTED;
    if(flags&4){
        if(size)return MOVIE_UNSUPPORTED;WebMovieReply reply=request(d,MOVIE_EOS,0,0,NULL,0);
        if(!reply.status){d->eos=1;d->client&=~(1u<<index);d->offered=0;}return reply.status;
    }
    if(!size)return MOVIE_INVALID;uint8_t *bytes=malloc((size_t)size);if(!bytes)return MOVIE_FAILED;
    web_read(d->input[index]+(uint32_t)offset,bytes,(size_t)size);WebMovieReply reply=request(d,MOVIE_SUBMIT,index,time,bytes,(size_t)size);free(bytes);
    if(!reply.status){d->client&=~(1u<<index);d->pending|=1u<<index;}return reply.status;
}
static int acquire_output(Decoder *d,uint64_t index,uint64_t time,uint64_t flags,uint64_t more){
    if(!index||!time||!flags||!more)return MOVIE_INVALID;wr32(index,UINT32_MAX);wr64(time,0);wr32(flags,0);wr32(more,0);
    if(!d->backend||!d->started||d->flushed)return MOVIE_NO_OUTPUT;
    unsigned slot=0;while(slot<d->output_count&&d->output[slot].used)slot++;if(slot==d->output_count)return MOVIE_NO_OUTPUT;
    uint8_t info[40];WebMovieReply reply=request(d,MOVIE_ACQUIRE,0,0,info,sizeof info);
    if(reply.status){if(reply.status==MOVIE_NO_OUTPUT)notifier_clear(d->owner,2);return reply.status;}
    Output out={.used=1,.backend=reply.value,.size=u32(info+4),.time=i64(info+8),.flags=u32(info+20),.width=u32(info+24),.height=u32(info+28),.stride=u32(info+32),.uv=u32(info+36)};
    d->output[slot]=out;wr32(index,slot);wr64(time,(uint64_t)out.time);wr32(flags,out.flags);wr32(more,u32(info+16));return MOVIE_OK;
}
static int output_buffer(Ctx *c,Decoder *d,int index,uint64_t buffer,uint64_t md){
    if(!buffer||index<0||(unsigned)index>=d->output_count||!d->output[index].used)return MOVIE_INVALID;Output *out=&d->output[index];
    uint64_t base=rd64(buffer+40),capacity=rd64(buffer+24);if(out->size&&(!base||capacity<out->size||out->size>BYTE_LIMIT))return MOVIE_INVALID;
    if(out->size){uint8_t *bytes=malloc(out->size);if(!bytes)return MOVIE_FAILED;int status=request(d,MOVIE_COPY,(int)out->backend,0,bytes,out->size).status;if(!status)web_write(base,bytes,out->size);free(bytes);if(status)return status;}
    wr32(buffer,0);wr32(buffer+4,out->size);wr64(buffer+32,(uint64_t)out->time);wr32(buffer+12,out->flags);
    return md?fill_format(c,d,md,out):MOVIE_OK;
}
int web_movie_decoder_import(Ctx *c,const char *name){
    if(!strcmp(name,"_ZN5movie7DecoderC1ENS_11DecoderTypeENS_19DecoderOutputFormatENS_11DecoderModeEi")||!strcmp(name,"_ZN5movie7DecoderC2ENS_11DecoderTypeENS_19DecoderOutputFormatENS_11DecoderModeEi")){decoder_construct(c);return 1;}return 0;
}
int web_movie_decoder_indirect(Ctx *c,uint64_t address){
    if(address<DECODER_FUNCTION||address>=DECODER_FUNCTION+64*16||(address&15))return 0;
    unsigned slot=(unsigned)((address-DECODER_FUNCTION)/16);uint64_t key=c->x[0],a=c->x[1],b=c->x[2],p=c->x[3],q=c->x[4];
    if(slot<2){decoder_destroy(c,key,slot==1);return 1;}
    Decoder *d=decoder_acquire(key);if(!d){c->x[0]=MOVIE_INVALID;return 1;}int status=MOVIE_INVALID;
    switch(slot){
        case 2:status=configure(c,d,a);break;
        case 3:{uint64_t events[4]={0};if(a)for(unsigned i=0;i<4;i++)events[i]=rd64(a+i*8);notifier_register(d->owner,events);status=MOVIE_OK;}break;
        case 4:status=fill_format(c,d,a,NULL);break;
        case 5:if(d->backend){if(!d->started||d->flushed){d->offered=255;d->client=d->pending=0;d->eos=0;}d->started=1;d->flushed=0;web_movie_notify(d->owner,4|(d->offered?1:0));status=MOVIE_OK;}break;
        case 6:case 7:case 8:status=halt(d,slot!=8);break;
        case 9:status=acquire_input(d,a,b);break;
        case 10:status=input_buffer(c,d,(int)a,b);break;
        case 11:status=send_input(d,(int)a,(int)b,(int)p,(int64_t)q,(unsigned)c->x[5]);break;
        case 12:status=acquire_output(d,a,b,p,q);break;
        case 13:status=output_buffer(c,d,(int)a,b,0);break;
        case 14:if(a&&d->backend){uint32_t size=d->type==5?(d->channels>4?8192*d->channels:32768):d->width*d->height+2*((d->width+1)/2)*((d->height+1)/2);wr32(a,size);if(b)wr32(b,1);status=MOVIE_OK;}break;
        case 15:if((int)a>=0&&a<d->output_count&&d->output[a].used){status=request(d,MOVIE_RELEASE,(int)d->output[a].backend,0,NULL,0).status;if(!status)memset(&d->output[a],0,sizeof d->output[a]);}break;
        case 16:status=d->last_error?d->last_error:d->backend?request(d,MOVIE_STATUS,0,0,NULL,0).status:MOVIE_FAILED;break;
        case 17:status=output_buffer(c,d,(int)a,b,p);break;
        case 18:if((int)a>=0&&a<d->output_count&&d->output[a].used)status=fill_format(c,d,b,&d->output[a]);break;
        default:status=MOVIE_UNSUPPORTED;break;
    }
    if(status!=MOVIE_NO_INPUT&&status!=MOVIE_NO_OUTPUT&&status!=MOVIE_INVALID&&status)d->last_error=status;
    decoder_release(d);c->x[0]=(uint64_t)status;return 1;
}
void web_movie_decoder_reset(void){
    /* Quiescent worker/GuestMemory teardown owns backing guest allocations. */
    pthread_mutex_lock(&decoder_lock);while(decoders){Decoder *next=decoders->next;pthread_mutex_destroy(&decoders->gate);free(decoders);decoders=next;}pthread_mutex_unlock(&decoder_lock);
    pthread_mutex_lock(&notify_lock);memset(notifications,0,sizeof notifications);pthread_mutex_unlock(&notify_lock);
}
unsigned web_movie_live_decoders(void){pthread_mutex_lock(&decoder_lock);unsigned count=0;for(Decoder *d=decoders;d;d=d->next)count++;pthread_mutex_unlock(&decoder_lock);return count;}
