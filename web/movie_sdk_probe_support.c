/* Test callbacks execute the SDK's actual ClientStreamReader ABI, copying
 * real WORKERFS file bytes into mapped guest storage. No host pointer crosses
 * into the SDK object, reader handle, callback args or movie::Buffer. */
#include "host_movie.h"
#include "movie_sdk_probe_support.h"
#include <stdio.h>
#include <stdlib.h>

#define BASE UINT64_C(0x8000000000)
#define EX (BASE+0x100)
#define MD (BASE+0x200)
#define SAMPLE_MD (MD+8)
#define BUFFER (BASE+0x300)
#define READER (BASE+0x400)
#define READER_TABLE (BASE+0x500)
#define PATH (BASE+0x600)
#define OUT (BASE+0x800)
#define FN UINT64_C(0x710000a000)
static GuestMemory memory;
static unsigned errors,reader_calls,reader_closed,deleted,source_closed,current;
static FILE *file;
static uint64_t storage;
static uint32_t capacity;
static uint8_t pages[8192];
typedef struct {uint64_t address;unsigned first,count;} Owned;
static Owned owned[128];
static WebMovieReply call(unsigned slot,uint64_t a,uint64_t b){
    Ctx c={0};c.x[0]=EX;c.x[1]=a;c.x[2]=b;c.x[19]=0x12345678;c.tpidr=UINT64_C(0x7800010000);c.fn=0x7100001234;c.x[30]=0x7100005678;
    if(!web_movie_codec_indirect(&c,rd64(rd64(EX)+slot*8))){errors++;return (WebMovieReply){MOVIE_FAILED,0};}
    if(c.x[19]!=0x12345678||c.tpidr!=UINT64_C(0x7800010000)||c.fn!=0x7100001234||c.x[30]!=0x7100005678)errors++;
    return (WebMovieReply){(int)c.x[0],0};
}
static void values(const char *name,uint64_t object,uint64_t a,uint64_t b){Ctx c={0};c.x[0]=object;c.x[1]=a;c.x[2]=b;if(!web_movie_import(&c,name))errors++;}
void call_indirect(Ctx *c,uint64_t function){
    uint64_t result=0;
    if(function==FN){
        uint64_t size=c->x[0];unsigned count=(unsigned)((size+4095)/4096),run=0;
        if(c->x[1]!=16||c->x[2]!=123||!count||count>8192)errors++;
        for(unsigned i=0;i<8192;i++){run=pages[i]?0:run+1;if(run==count){unsigned first=i+1-count;for(unsigned j=0;j<128;j++)if(!owned[j].address){result=BASE+0x100000+(uint64_t)first*4096;if(gm_commit(&memory,result,(size_t)count*4096)!=GM_OK){result=0;break;}memset(pages+first,1,count);owned[j]=(Owned){result,first,count};break;}break;}}
    }else if(function==FN+16){
        if(c->x[1]!=123)errors++;unsigned found=0;for(unsigned i=0;i<128;i++)if(owned[i].address==c->x[0]){gm_decommit(&memory,owned[i].address,(size_t)owned[i].count*4096);memset(pages+owned[i].first,0,owned[i].count);owned[i].address=0;found=1;break;}if(!found)errors++;
    }else if(function==FN+32){
        if(c->x[0]!=READER||rd8(c->x[1])!='l')errors++;file=fopen("/movie/input.webm","rb");if(file){wr64(c->x[2],0xcafe);result=1;}
    }else if(function==FN+48){
        if(c->x[0]!=READER||c->x[1]!=0xcafe||!file)errors++;
        if(file){fseek(file,(long)c->x[2],SEEK_SET);size_t remaining=c->x[4]>173123?173123:(size_t)c->x[4];uint8_t bytes[8192];
            while(remaining){size_t n=fread(bytes,1,remaining>sizeof bytes?sizeof bytes:remaining,file);if(!n)break;web_write(c->x[3]+result,bytes,n);result+=n;remaining-=n;}
        }reader_calls++;
    }else if(function==FN+64){if(c->x[0]!=READER||c->x[1]!=0xcafe||!file)errors++;if(file)fclose(file);file=NULL;reader_closed++;result=1;
    }else if(function==FN+80){if(c->x[0]!=READER||c->x[1]!=0xcafe||!file)errors++;if(file){fseek(file,0,SEEK_END);wr64(c->x[2],(uint64_t)ftell(file));}result=0x123; /* SDK ignores GetSize's result */
    }else if(function==UINT64_C(0x710392e590)){deleted++;}
    else {guest_trap("unexpected SDK probe callback",function);}
    memset(c,0xa5,sizeof *c);c->x[0]=result;
}
int movie_sdk_probe_begin(void){
    gm_init_limit(&memory,16384);if(gm_reserve(&memory,BASE,64*1024*1024)!=GM_OK||gm_commit(&memory,BASE,65536)!=GM_OK||gm_reserve(&memory,UINT64_C(0x7800000000),4096)!=GM_OK||gm_commit(&memory,UINT64_C(0x7800000000),4096)!=GM_OK)return 1;
    web_memory_bind(&memory);web_movie_reset();web_movie_codec_reset();
    Ctx c={0};c.x[0]=FN;c.x[1]=FN+16;c.x[3]=123;if(!web_movie_import(&c,"_ZN5movie12SetAllocatorEPFPvmmS0_EPFvS0_S0_EPFS0_S0_mS0_ES0_"))errors++;
    wr64(READER,READER_TABLE);for(unsigned i=0;i<4;i++)wr64(READER_TABLE+16+i*8,FN+32+i*16);web_write(PATH,"local.webm",11);
    values("_ZN5movie9MediaDataC1Ev",MD,0,0);values("_ZN5movie9MediaDataC1Ev",SAMPLE_MD,0,0);values("_ZN5movie6BufferC1Ev",BUFFER,0,0);return 0;
}
WebMovieReply movie_sdk_probe_open(uint32_t *backend){
    Ctx c={0};c.x[0]=EX;c.x[1]=3;if(!web_movie_codec_import(&c,"_ZN5movie9ExtractorC1ENS_13ContainerTypeE"))return (WebMovieReply){MOVIE_FAILED,0};
    WebMovieReply r=call(15,READER,PATH);if(r.status)return r;r=call(4,OUT,0);if(r.status)return r;
    r.value=rd32(OUT);*backend=(uint32_t)rd64(EX+8);return r;
}
static uint64_t number(const char *key,unsigned type){uint64_t value=0;if(!web_movie_metadata_get(MD,key,type,&value))errors++;return value;}
static void put32(void *info,unsigned offset,uint32_t value){memcpy((uint8_t *)info+offset,&value,4);}
WebMovieReply movie_sdk_probe_track(int index,void *info){
    WebMovieReply r=call(5,(uint64_t)index,MD);if(r.status)return r;memset(info,0,64);
    if(number("nn-extractor",WEB_MOVIE_POINTER)!=rd64(EX+8)||number("nn-track",WEB_MOVIE_I32)!=(uint64_t)index)errors++;
    uint64_t width=0;if(web_movie_metadata_get(MD,"width",WEB_MOVIE_I32,&width)){put32(info,4,1);put32(info,8,(uint32_t)width);put32(info,12,(uint32_t)number("height",WEB_MOVIE_I32));put32(info,44,1);
        if(rd8(number("mime",WEB_MOVIE_STRING))!='v')errors++;
        if(storage)web_movie_guest_free(&(Ctx){0},storage);capacity=(uint32_t)number("max-input-size",WEB_MOVIE_I32);put32(info,24,capacity);storage=web_movie_guest_alloc(&(Ctx){0},capacity);if(!storage)return (WebMovieReply){MOVIE_FAILED,0};values("_ZN5movie6BufferC1EPvm",BUFFER,storage,capacity);
    }
    uint64_t duration=number("durationUs",WEB_MOVIE_I64);memcpy((uint8_t *)info+32,&duration,8);return r;
}
WebMovieReply movie_sdk_probe_select(int index){
    WebMovieReply r=call(6,(uint64_t)index,0);if(r.status)return r;
    if(call(14,OUT,OUT+8).status||rd64(OUT)!=0||rd64(OUT+8)==0)errors++;
    if(call(8,0,0).status)errors++;return r;
}
WebMovieReply movie_sdk_probe_peek(void *sample){
    WebMovieReply r=call(9,OUT,0);if(r.status)return r;uint32_t track=rd32(OUT);r=call(12,OUT+8,0);if(r.status)return r;
    if(!current){
        wr64(BUFFER+24,1);if(call(10,BUFFER,0).status!=MOVIE_INVALID)errors++;wr64(BUFFER+24,capacity);
        r=call(10,BUFFER,0);if(r.status)return r;if(call(10,BUFFER,0).status!=MOVIE_INVALID)errors++;current=1;
    }
    memset(sample,0,24);uint32_t size=rd32(BUFFER+4);memcpy(sample,&track,4);memcpy((uint8_t *)sample+4,&size,4);uint64_t time=rd64(OUT+8);memcpy((uint8_t *)sample+8,&time,8);
    if(call(13,SAMPLE_MD,0).status)errors++;uint64_t v=0;
    if(!web_movie_metadata_get(SAMPLE_MD,"timeUs",WEB_MOVIE_I64,&v)||v!=time)errors++;
    if(!web_movie_metadata_get(SAMPLE_MD,"track-index",WEB_MOVIE_I32,&v)||v!=track)errors++;
    if(!web_movie_metadata_get(SAMPLE_MD,"is-sync-frame",WEB_MOVIE_I32,&v)||v>1)errors++;uint32_t key=(uint32_t)v;memcpy((uint8_t *)sample+16,&key,4);return r;
}
WebMovieReply movie_sdk_probe_read(void *bytes,size_t size){uint32_t n=rd32(BUFFER+4);if(!current||size<n)return (WebMovieReply){MOVIE_INVALID,0};web_read(rd64(BUFFER+40),bytes,n);return (WebMovieReply){0,0};}
WebMovieReply movie_sdk_probe_advance(void){WebMovieReply r=call(11,0,0);if(!r.status)current=0;return r;}
WebMovieReply movie_sdk_probe_close(void){call(0,0,0);source_closed=1;return (WebMovieReply){0,0};}
unsigned movie_sdk_probe_reader_calls(void){return reader_calls;}
int movie_sdk_probe_finish(void){
    if(!source_closed)movie_sdk_probe_close();if(reader_closed!=1||reader_calls<2||file)errors++;
    Ctx c={0};c.x[0]=EX+16;c.x[1]=3;web_movie_codec_import(&c,"_ZN5movie9ExtractorC2ENS_13ContainerTypeE");c.x[0]=EX+16;if(!web_movie_codec_indirect(&c,rd64(rd64(EX+16)+8))||deleted!=1)errors++;
    values("_ZN5movie9MediaDataD1Ev",MD,0,0);values("_ZN5movie9MediaDataD1Ev",SAMPLE_MD,0,0);values("_ZN5movie6BufferD1Ev",BUFFER,0,0);if(storage)web_movie_guest_free(&(Ctx){0},storage);
    if(web_movie_live_allocations()||web_movie_live_metadata()||web_movie_live_extractors())errors++;
    web_movie_codec_reset();web_movie_reset();web_memory_bind(NULL);gm_destroy(&memory);return (int)errors;
}
