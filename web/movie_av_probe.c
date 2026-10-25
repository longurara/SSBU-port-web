/* Real interleaved VP9/Vorbis through production OS router, Extractor, Decoder
 * buffers/events and guest allocators. Fixture input has natural finite EOF. */
#include "host_movie.h"
#include "movie_sdk_probe_support.h"
#include <emscripten/emscripten.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define BASE UINT64_C(0x8000000000)
#define DEC(i) (BASE+0x2000+(i)*256)
#define BUF(i) (BASE+0x2200+(i)*256)
#define FMT(i) (BASE+0x2400+(i)*256)
#define OUT (BASE+0x2600)
#define EVENTS(i) (BASE+0x2800+(i)*64)
#define EVENT(i,j) (BASE+0x3000+(i)*256+(j)*32)
int web_os_import(Ctx *,const char *);
int web_os_indirect(Ctx *,uint64_t);
static pthread_t driver;static unsigned checks,failures,first_failure,limit;
static char report[131072];
typedef struct {uint64_t time;unsigned bytes,hash;} Frame;
static Frame frames[2][512];
void guest_trap(const char *what,uint64_t address){fprintf(stderr,"movie AV trap: %s %llx\n",what,(unsigned long long)address);abort();}
#define check(v) do{checks++;if(!(v)){failures++;if(!first_failure)first_failure=__LINE__;}}while(0)
static int call(uint64_t key,unsigned slot,uint64_t a,uint64_t b,uint64_t p,uint64_t q,uint64_t r){Ctx c={0};c.x[0]=key;c.x[1]=a;c.x[2]=b;c.x[3]=p;c.x[4]=q;c.x[5]=r;c.x[19]=0x12345678;check(web_os_indirect(&c,rd64(rd64(key)+slot*8)));check(c.x[19]==0x12345678);return (int)c.x[0];}
static uint64_t os(const char *name,uint64_t key,uint64_t a,uint64_t b){Ctx c={0};c.x[0]=key;c.x[1]=a;c.x[2]=b;check(web_os_import(&c,name));return c.x[0];}
static void value(const char *name,uint64_t key,uint64_t a,uint64_t b){Ctx c={0};c.x[0]=key;c.x[1]=a;c.x[2]=b;check(web_movie_import(&c,name));}
static uint32_t u32(const void *p){uint32_t v;memcpy(&v,p,4);return v;}
static uint32_t hash(uint64_t base,unsigned size,unsigned *nonzero){uint8_t data[4096];uint32_t h=2166136261u;for(unsigned at=0;at<size;){unsigned n=size-at>sizeof data?sizeof data:size-at;web_read(base+at,data,n);for(unsigned i=0;i<n;i++){h=(h^data[i])*16777619u;if(data[i])(*nonzero)++;}at+=n;}return h;}
static void *run(void *unused){
 (void)unused;unsigned initialized=0,created=0,eof=0,waits=0,counts[2]={0},fed[2]={0},eos[2]={0},sent[2]={0},held[2][64],held_n[2]={0},max_held[2]={0},capacity[2]={0},nonzero[2]={0};int tracks[2]={-1,-1};uint64_t data[2]={0};uint8_t info[64];uint32_t source=0;
 check(movie_sdk_probe_begin()==0);initialized=1;WebMovieReply r=movie_sdk_probe_open(&source);check(!r.status);if(r.status)goto finish;
 unsigned total=r.value;for(unsigned i=0;i<total;i++){r=movie_sdk_probe_track((int)i,info);check(!r.status);if(r.status)goto finish;if(u32(info+4)==1)tracks[0]=(int)i;if(u32(info)==5)tracks[1]=(int)i;}
 check(tracks[0]>=0&&tracks[1]>=0);if(tracks[0]<0||tracks[1]<0)goto finish;
 for(unsigned i=0;i<2;i++){
  check(movie_sdk_probe_track(tracks[i],info).status==0);
  Ctx c={0};c.x[0]=DEC(i);c.x[1]=i?5:u32(info);c.x[2]=i?1:4;c.x[3]=1;c.x[4]=1;
  check(web_os_import(&c,"_ZN5movie7DecoderC1ENS_11DecoderTypeENS_19DecoderOutputFormatENS_11DecoderModeEi"));created++;
  int status=call(DEC(i),2,movie_sdk_probe_metadata(),0,0,0,0);check(!status);if(status)goto finish;
  check(call(movie_sdk_probe_extractor(),6,(unsigned)tracks[i],0,0,0,0)==0);
  value("_ZN5movie6BufferC1Ev",BUF(i),0,0);value("_ZN5movie9MediaDataC1Ev",FMT(i),0,0);
  check(call(DEC(i),14,OUT,OUT+4,0,0,0)==0);capacity[i]=rd32(OUT);data[i]=web_movie_guest_alloc(&c,capacity[i]);check(data[i]!=0);if(!data[i])goto finish;
  for(unsigned j=0;j<4;j++){os("nnosInitializeEvent",EVENT(i,j),0,1);wr64(EVENTS(i)+j*8,EVENT(i,j));}
  check(call(DEC(i),3,EVENTS(i),0,0,0,0)==0);check(call(DEC(i),5,0,0,0,0,0)==0);
  check(os("_ZN2nn2os12TryWaitEventEPNS0_9EventTypeE",EVENT(i,2),0,0)==1);
  check(call(DEC(i),4,FMT(i),0,0,0,0)==0);
  if(i){uint64_t rate=0,channels=0,pcm=0;check(web_movie_metadata_get(FMT(i),"sample-rate",WEB_MOVIE_I32,&rate)&&rate==48000);check(web_movie_metadata_get(FMT(i),"channel-count",WEB_MOVIE_I32,&channels)&&channels==2);check(web_movie_metadata_get(FMT(i),"pcm-encoding",WEB_MOVIE_I32,&pcm)&&pcm==2);}
 }
 while(!(eos[0]&&eos[1])){
  unsigned progress=0;
  while(!eof){
   int status=call(movie_sdk_probe_extractor(),9,OUT,0,0,0,0);if(status==MOVIE_END){eof=1;progress=1;break;}check(!status);if(status)goto finish;
   unsigned index=rd32(OUT);int selected=index==(unsigned)tracks[0]?0:index==(unsigned)tracks[1]?1:-1;check(selected>=0);if(selected<0)goto finish;unsigned i=(unsigned)selected;
   status=call(DEC(i),9,OUT,OUT+4,0,0,0);if(status==MOVIE_NO_INPUT)break;check(!status);if(status)goto finish;unsigned input=rd32(OUT);
   check(call(DEC(i),10,input,BUF(i),0,0,0)==0);check(call(movie_sdk_probe_extractor(),10,BUF(i),0,0,0,0)==0);
   check(call(movie_sdk_probe_extractor(),12,OUT+8,0,0,0,0)==0);
   status=call(DEC(i),11,input,rd32(BUF(i)),rd32(BUF(i)+4),rd64(OUT+8),0);check(!status);if(status)goto finish;
   check(call(movie_sdk_probe_extractor(),11,0,0,0,0,0)==0);fed[i]++;progress=1;
  }
  for(unsigned i=0;i<2;i++){
   if(eof&&!sent[i]){int status=call(DEC(i),9,OUT,OUT+4,0,0,0);if(!status){check(call(DEC(i),11,rd32(OUT),0,0,0,4)==0);sent[i]=1;progress=1;}else check(status==MOVIE_NO_INPUT);}
   for(;;){
    int status=call(DEC(i),12,OUT,OUT+8,OUT+16,OUT+20,0);if(status==MOVIE_NO_OUTPUT)break;check(!status);if(status)goto finish;unsigned slot=rd32(OUT),flags=rd32(OUT+16);uint64_t time=rd64(OUT+8);
    check(slot<(i?64u:48u));progress=1;
    if(flags&4){eos[i]++;check(eof&&sent[i]&&counts[i]>0);if(counts[i])check(time>=frames[i][counts[i]-1].time);check(call(DEC(i),15,slot,0,0,0,0)==0);break;}
    check(counts[i]<512);if(counts[i]>=512)goto finish;
    value("_ZN5movie6BufferC1EPvm",BUF(i),data[i],capacity[i]);check(call(DEC(i),17,slot,BUF(i),FMT(i),0,0)==0);
    unsigned bytes=rd32(BUF(i)+4);check(bytes>0&&bytes<=capacity[i]&&rd64(BUF(i)+32)==time);
    if(counts[i])check(time>=frames[i][counts[i]-1].time);
    frames[i][counts[i]++]=(Frame){time,bytes,hash(data[i],bytes,&nonzero[i])};held[i][held_n[i]++]=slot;if(held_n[i]>max_held[i])max_held[i]=held_n[i];
   }
   if(held_n[i]==(i?64u:48u)||eof){unsigned n=eof?held_n[i]:1;for(unsigned j=0;j<n;j++){unsigned slot=held[i][0];memmove(held[i],held[i]+1,(--held_n[i])*sizeof *held[i]);check(call(DEC(i),15,slot,0,0,0,0)==0);}if(n)progress=1;}
   check(call(DEC(i),16,0,0,0,0,0)==0);
  }
  if(!progress){os("_ZN2nn2os14TimedWaitEventEPNS0_9EventTypeENS_8TimeSpanE",EVENT(0,1),10000000,0);if(++waits>3000){check(0);goto finish;}}
 }
 check(fed[0]==limit&&counts[0]==fed[0]&&counts[1]+1==fed[1]&&eos[0]==1&&eos[1]==1&&nonzero[1]>0);
finish:
 for(unsigned i=0;i<created;i++){call(DEC(i),3,0,0,0,0,0);call(DEC(i),0,0,0,0,0,0);value("_ZN5movie6BufferD1Ev",BUF(i),0,0);value("_ZN5movie9MediaDataD1Ev",FMT(i),0,0);if(data[i])web_movie_guest_free(&(Ctx){0},data[i]);for(unsigned j=0;j<4;j++)if(capacity[i])os("nnosFinalizeEvent",EVENT(i,j),0,0);}
 if(initialized)check(movie_sdk_probe_finish()==0);
 size_t pos=(size_t)snprintf(report,sizeof report,"{\"ok\":%s,\"checks\":%u,\"failures\":%u,\"firstFailureLine\":%u,\"fedVideo\":%u,\"fedAudio\":%u,\"eof\":%s,\"eosVideo\":%u,\"eosAudio\":%u,\"maxHeldVideo\":%u,\"maxHeldAudio\":%u,\"audioNonzeroBytes\":%u,\"waits\":%u",failures?"false":"true",checks,failures,first_failure,fed[0],fed[1],eof?"true":"false",eos[0],eos[1],max_held[0],max_held[1],nonzero[1],waits);
 for(unsigned i=0;i<2;i++){pos+=(size_t)snprintf(report+pos,sizeof report-pos,",\"%sFrames\":[",i?"audio":"video");for(unsigned j=0;j<counts[i];j++)pos+=(size_t)snprintf(report+pos,sizeof report-pos,"%s{\"timestamp\":%llu,\"bytes\":%u,\"hash\":\"%x\"}",j?",":"",(unsigned long long)frames[i][j].time,frames[i][j].bytes,frames[i][j].hash);pos+=(size_t)snprintf(report+pos,sizeof report-pos,"]");}
 snprintf(report+pos,sizeof report-pos,"}");MAIN_THREAD_ASYNC_EM_ASM({Module['onMovieProbeReport'](UTF8ToString($0));},report);return NULL;
}
EMSCRIPTEN_KEEPALIVE int movie_bridge_start(unsigned count,unsigned eof){if(!eof||count<2||count>128)return 1;limit=count;return pthread_create(&driver,NULL,run,NULL);}
EMSCRIPTEN_KEEPALIVE int movie_bridge_join(void){return pthread_join(driver,NULL);}
