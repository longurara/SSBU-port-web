/* SDK Extractor virtual calls. Object implementations stay on the host;
 * reader handles, buffers and callback arguments remain guest addresses. */
#include "host_movie.h"
#include "host_movie_bridge.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#define EXTRACTOR_VTABLE UINT64_C(0x7800000a00)
#define EXTRACTOR_FUNCTION UINT64_C(0x7500003000)
/* Same version-pinned guest delete routine used by runtime/nnmovie_mf.c. The
 * game allocates SDK wrappers itself; they are not movie allocator buffers. */
#define GAME_DELETE UINT64_C(0x710392e590)
#define SOURCE_LIMIT (512u*1024u*1024u)
#define SOURCE_CHUNK 1048576u
typedef struct Extractor {
    uint64_t key;uint32_t backend,tracks;int64_t duration;unsigned references;
    int pending,consumed,selected;uint8_t sample[24];atomic_int closing;
    pthread_mutex_t gate;struct Extractor *next;
} Extractor;
static pthread_mutex_t codec_lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t codec_changed=PTHREAD_COND_INITIALIZER;
static Extractor *extractors;
static uint32_t u32(const void *p){uint32_t value;memcpy(&value,p,4);return value;}
static int64_t i64(const void *p){int64_t value;memcpy(&value,p,8);return value;}
static WebMovieReply request(unsigned op,Extractor *e,int index,unsigned flags,int64_t time,void *data,size_t size){return web_movie_request(op,e->backend,index,flags,time,data,size);}
static void extractor_release(Extractor *e){
    pthread_mutex_unlock(&e->gate);pthread_mutex_lock(&codec_lock);e->references--;pthread_cond_broadcast(&codec_changed);pthread_mutex_unlock(&codec_lock);
}
static Extractor *extractor_acquire(uint64_t object){
    pthread_mutex_lock(&codec_lock);Extractor *e=extractors;
    while(e&&(e->key!=object||atomic_load(&e->closing)))e=e->next;
    if(e)e->references++;pthread_mutex_unlock(&codec_lock);
    if(e){pthread_mutex_lock(&e->gate);if(atomic_load(&e->closing)){extractor_release(e);return NULL;}}
    return e;
}
static void extractor_destroy(Ctx *c,uint64_t object,int deleting){
    pthread_mutex_lock(&codec_lock);Extractor **link=&extractors;while(*link&&(*link)->key!=object)link=&(*link)->next;
    Extractor *e=*link;
    if(e){atomic_store(&e->closing,1);*link=e->next;while(e->references)pthread_cond_wait(&codec_changed,&codec_lock);}
    pthread_mutex_unlock(&codec_lock);
    if(e){if(e->backend)request(MOVIE_CLOSE,e,0,0,0,NULL,0);pthread_mutex_destroy(&e->gate);free(e);}
    wr64(object+8,0);
    if(deleting)web_movie_guest_call(c,GAME_DELETE,object,0,0,0,0);
}
static void extractor_construct(Ctx *c){
    Extractor *e=calloc(1,sizeof *e);if(!e)guest_trap("movie Extractor allocation failed",c->x[0]);
    e->key=c->x[0];e->selected=-1;pthread_mutex_init(&e->gate,NULL);atomic_init(&e->closing,0);
    WebMovieReply reply=web_movie_request(MOVIE_CREATE,0,0,0,0,NULL,0);if(!reply.status)e->backend=reply.value;
    pthread_mutex_lock(&codec_lock);e->next=extractors;extractors=e;pthread_mutex_unlock(&codec_lock);
    for(unsigned i=0;i<64;i++)wr64(EXTRACTOR_VTABLE+i*8,EXTRACTOR_FUNCTION+i*16);
    wr64(e->key,EXTRACTOR_VTABLE);wr64(e->key+8,e->backend);
}
static int source_begin(Extractor *e){
    if(e->backend)request(MOVIE_CLOSE,e,0,0,0,NULL,0);
    WebMovieReply reply=web_movie_request(MOVIE_CREATE,0,0,0,0,NULL,0);
    e->backend=reply.status?0:reply.value;e->tracks=0;e->duration=0;e->pending=0;e->consumed=0;e->selected=-1;
    wr64(e->key+8,e->backend);return reply.status;
}
static int source_open(Extractor *e){
    WebMovieReply reply=request(MOVIE_OPEN,e,0,0,0,NULL,0);if(reply.status)return reply.status;
    e->tracks=reply.value;uint8_t info[64];reply=request(MOVIE_TRACK,e,0,0,0,info,sizeof info);
    if(!reply.status)e->duration=i64(info+32);return reply.status;
}
static int source_memory(Extractor *e,uint64_t address,int64_t size){
    if(!address||size<1||size>SOURCE_LIMIT||address>UINT64_MAX-(uint64_t)size)return MOVIE_INVALID;
    int status=source_begin(e);if(status)return status;
    uint8_t *data=malloc(SOURCE_CHUNK);if(!data)return MOVIE_FAILED;
    for(int64_t offset=0;offset<size;){size_t n=(size-offset)>SOURCE_CHUNK?SOURCE_CHUNK:(size_t)(size-offset);web_read(address+(uint64_t)offset,data,n);WebMovieReply r=request(MOVIE_APPEND,e,0,0,0,data,n);if(r.status){free(data);return r.status;}offset+=(int64_t)n;}
    free(data);return source_open(e);
}
static int source_reader(Ctx *c,Extractor *e,uint64_t reader,uint64_t path){
    if(!reader||!path)return MOVIE_INVALID;
    uint64_t scratch=web_movie_guest_alloc(c,SOURCE_CHUNK+64);if(!scratch)return MOVIE_FAILED;
    uint8_t *data=malloc(SOURCE_CHUNK);if(!data){web_movie_guest_free(c,scratch);return MOVIE_FAILED;}
    uint64_t table=rd64(reader),handle=0;int status=MOVIE_FAILED,opened=0;wr64(scratch,0);wr64(scratch+8,0);
    if(!(web_movie_guest_call(c,rd64(table+16),reader,path,scratch,0,0)&1))goto finish;
    opened=1;handle=rd64(scratch);
    web_movie_guest_call(c,rd64(table+40),reader,handle,scratch+8,0,0);
    int64_t size=(int64_t)rd64(scratch+8);if(size<1||size>SOURCE_LIMIT)goto finish;
    status=source_begin(e);if(status)goto finish;
    for(int64_t offset=0;offset<size;){
        size_t n=(size-offset)>SOURCE_CHUNK?SOURCE_CHUNK:(size_t)(size-offset);
        int64_t got=(int64_t)web_movie_guest_call(c,rd64(table+24),reader,handle,(uint64_t)offset,scratch+64,n);
        if(got<1||got>(int64_t)n){status=MOVIE_FAILED;goto finish;}
        web_read(scratch+64,data,(size_t)got);WebMovieReply reply=request(MOVIE_APPEND,e,0,0,0,data,(size_t)got);if(reply.status){status=reply.status;goto finish;}offset+=got;
    }
finish:
    if(opened)web_movie_guest_call(c,rd64(table+32),reader,handle,0,0,0);
    free(data);web_movie_guest_free(c,scratch);
    /* The backend owns the appended bytes. Release the game's reader/scratch
     * before parsing metadata, matching the native adapter's reader lifetime. */
    return status?status:source_open(e);
}
static int source_path(Extractor *e,uint64_t address){
    char path[512];unsigned i;for(i=0;i<sizeof path;i++){path[i]=(char)rd8(address+i);if(!path[i])break;}if(i==sizeof path)return MOVIE_INVALID;
    FILE *file=fopen(path,"rb");if(!file)return MOVIE_FAILED;int status=source_begin(e);uint8_t *data=malloc(SOURCE_CHUNK);
    if(!data)status=MOVIE_FAILED;
    if(!status){size_t n;uint64_t total=0;while((n=fread(data,1,SOURCE_CHUNK,file))>0){total+=n;if(total>SOURCE_LIMIT){status=MOVIE_INVALID;break;}WebMovieReply r=request(MOVIE_APPEND,e,0,0,0,data,n);if(r.status){status=r.status;break;}}if(ferror(file))status=MOVIE_FAILED;}
    fclose(file);free(data);return status?status:source_open(e);
}
static int peek(Extractor *e){WebMovieReply r=request(MOVIE_PEEK,e,0,0,0,e->sample,sizeof e->sample);e->pending=r.status==0;return r.status;}
static int track_configuration(Ctx *c,Extractor *e,int index,uint64_t out){
    if(!out)return MOVIE_INVALID;uint8_t info[64];WebMovieReply r=request(MOVIE_TRACK,e,index,0,0,info,sizeof info);if(r.status)return r.status;
    web_movie_metadata_clear(c,out);
    const char *mime=u32(info)==3?"video/x-vnd.on2.vp9":u32(info)==2?"video/x-vnd.on2.vp8":u32(info)==5?"audio/vorbis":u32(info)==6?"audio/opus":"application/octet-stream";
    int ok=web_movie_metadata_string(c,out,"mime",mime);
    ok&=web_movie_metadata_set(c,out,"durationUs",WEB_MOVIE_I64,(uint64_t)i64(info+32));
    ok&=web_movie_metadata_set(c,out,"max-input-size",WEB_MOVIE_I32,u32(info+24));
    if(u32(info+4)==1){ok&=web_movie_metadata_set(c,out,"width",WEB_MOVIE_I32,u32(info+8));ok&=web_movie_metadata_set(c,out,"height",WEB_MOVIE_I32,u32(info+12));ok&=web_movie_metadata_set(c,out,"frame-rate",WEB_MOVIE_I32,u32(info+28));}
    else {ok&=web_movie_metadata_set(c,out,"sample-rate",WEB_MOVIE_I32,u32(info+16));ok&=web_movie_metadata_set(c,out,"channel-count",WEB_MOVIE_I32,u32(info+20));}
    ok&=web_movie_metadata_set(c,out,"nn-extractor",WEB_MOVIE_POINTER,e->backend);ok&=web_movie_metadata_set(c,out,"nn-track",WEB_MOVIE_I32,(uint32_t)index);
    return ok?MOVIE_OK:MOVIE_FAILED;
}
int web_movie_codec_import(Ctx *c,const char *name){
    if(!strcmp(name,"_ZN5movie9ExtractorC1ENS_13ContainerTypeE")||!strcmp(name,"_ZN5movie9ExtractorC2ENS_13ContainerTypeE")){extractor_construct(c);return 1;}
    return 0;
}
int web_movie_codec_indirect(Ctx *c,uint64_t address){
    if(address<EXTRACTOR_FUNCTION||address>=EXTRACTOR_FUNCTION+64*16||(address&15))return 0;
    unsigned slot=(unsigned)((address-EXTRACTOR_FUNCTION)/16);uint64_t object=c->x[0],a=c->x[1],b=c->x[2];
    if(slot<2){extractor_destroy(c,object,slot==1);return 1;}
    Extractor *e=extractor_acquire(object);if(!e){c->x[0]=MOVIE_INVALID;return 1;}int status=MOVIE_INVALID;
    switch(slot){
        case 2:status=a?source_path(e,a):MOVIE_INVALID;break;
        case 3:status=source_memory(e,a,(int64_t)(int32_t)b);break;
        case 4:if(a){wr32(a,e->tracks);status=e->tracks?MOVIE_OK:MOVIE_FAILED;}break;
        case 5:status=track_configuration(c,e,(int)a,b);break;
        case 6:case 7:status=request(MOVIE_SELECT,e,(int)a,slot==6,0,NULL,0).status;if(!status&&((slot==6&&e->selected!=(int)a)||(slot==7&&e->selected==(int)a))){e->selected=slot==6?(int)a:-1;e->pending=0;e->consumed=0;}break;
        case 8:status=request(MOVIE_SEEK,e,0,0,(int64_t)a,NULL,0).status;if(!status){e->pending=0;e->consumed=0;}break;
        case 9:if(a){status=peek(e);if(!status)wr64(a,u32(e->sample));}break;
        case 10:if(a){
            status=peek(e);if(status)break;uint64_t base=rd64(a+40),capacity=rd64(a+24);uint32_t size=u32(e->sample+4);
            if(!base||capacity<size||e->consumed){status=MOVIE_INVALID;break;}
            uint8_t *bytes=malloc(size);if(!bytes){status=MOVIE_FAILED;break;}
            status=request(MOVIE_READ,e,0,0,0,bytes,size).status;
            if(!status){web_write(base,bytes,size);wr32(a,0);wr32(a+4,size);wr64(a+32,(uint64_t)i64(e->sample+8));e->consumed=1;}free(bytes);
        }break;
        case 11:status=request(MOVIE_ADVANCE,e,0,0,0,NULL,0).status;if(!status){e->pending=0;e->consumed=0;}break;
        case 12:if(a){status=peek(e);wr64(a,status?UINT64_MAX:(uint64_t)i64(e->sample+8));}break;
        case 13:if(a){web_movie_metadata_clear(c,a);status=MOVIE_OK;if(e->pending){status=web_movie_metadata_set(c,a,"timeUs",WEB_MOVIE_I64,(uint64_t)i64(e->sample+8))&&web_movie_metadata_set(c,a,"track-index",WEB_MOVIE_I32,u32(e->sample))&&web_movie_metadata_set(c,a,"is-sync-frame",WEB_MOVIE_I32,u32(e->sample+16))?MOVIE_OK:MOVIE_FAILED;}}break;
        case 14:if(a)wr64(a,0);if(b)wr64(b,(uint64_t)e->duration);status=e->tracks?MOVIE_OK:MOVIE_FAILED;break;
        case 15:status=source_reader(c,e,a,b);break;
        default:status=MOVIE_UNSUPPORTED;break;
    }
    extractor_release(e);c->x[0]=(uint64_t)status;return 1;
}
void web_movie_codec_reset(void){
    /* Worker teardown has already stopped all callers and closes the JS backend. */
    pthread_mutex_lock(&codec_lock);while(extractors){Extractor *next=extractors->next;pthread_mutex_destroy(&extractors->gate);free(extractors);extractors=next;}pthread_mutex_unlock(&codec_lock);
}
unsigned web_movie_live_extractors(void){
    pthread_mutex_lock(&codec_lock);unsigned count=0;for(Extractor *e=extractors;e;e=e->next)count++;pthread_mutex_unlock(&codec_lock);return count;
}
