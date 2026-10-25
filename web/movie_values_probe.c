/* Guest ABI and allocator ownership checks through production movie imports. */
#include "host_movie.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#define BASE UINT64_C(0x8000000000)
#define ALLOC_A UINT64_C(0x710000a000)
#define FREE_A UINT64_C(0x710000a010)
#define ALLOC_B UINT64_C(0x710000b000)
#define FREE_B UINT64_C(0x710000b010)
static GuestMemory memory;
static pthread_mutex_t slots_lock=PTHREAD_MUTEX_INITIALIZER;
static uint64_t slots[128];
static atomic_uint checks,failures,alloc_a,alloc_b,free_a,free_b,fail_allocate;
static char report[256];
static void check_line(int value,int line){atomic_fetch_add(&checks,1);if(!value){atomic_fetch_add(&failures,1);fprintf(stderr,"movie value check failed at line%d\n",line);}}
#define check(v) check_line((v),__LINE__)
void guest_trap(const char *what,uint64_t address){fprintf(stderr,"movie probe trap: %s %llx\n",what,(unsigned long long)address);abort();}
static uint64_t invoke(const char *name,uint64_t object,uint64_t a,uint64_t b){Ctx c={0};c.x[0]=object;c.x[1]=a;c.x[2]=b;check(web_movie_import(&c,name));return c.x[0];}
static void allocator_set(uint64_t alloc,uint64_t release,uint64_t user){Ctx c={0};c.x[0]=alloc;c.x[1]=release;c.x[3]=user;check(web_movie_import(&c,"_ZN5movie12SetAllocatorEPFPvmmS0_EPFvS0_S0_EPFS0_S0_mS0_ES0_"));}
void call_indirect(Ctx *c,uint64_t function){
    uint64_t result=0;
    if(function==ALLOC_A||function==ALLOC_B){
        check(c->x[0]<=8192&&c->x[1]==16&&c->x[2]==(function==ALLOC_A?111:222));
        /* This reentry would deadlock if production held its registry lock. */
        check(invoke("_ZNK5movie6Buffer4BaseEv",BASE+0x800,0,0)==BASE+0x900);
        if(!atomic_load(&fail_allocate)){
            pthread_mutex_lock(&slots_lock);for(unsigned i=0;i<128;i++)if(!slots[i]){result=BASE+0x10000+(uint64_t)i*8192;slots[i]=function;break;}pthread_mutex_unlock(&slots_lock);
            check(result!=0);if(result)wr32(result,0xaabbccdd);
        }
        atomic_fetch_add(function==ALLOC_A?&alloc_a:&alloc_b,1);
    }else if(function==FREE_A||function==FREE_B){
        check(c->x[1]==(function==FREE_A?111:222));
        uint64_t offset=c->x[0]-BASE-0x10000,index=offset/8192;check(offset%8192==0&&index<128);
        pthread_mutex_lock(&slots_lock);if(index<128){check(slots[index]==(function==FREE_A?ALLOC_A:ALLOC_B));slots[index]=0;}pthread_mutex_unlock(&slots_lock);
        atomic_fetch_add(function==FREE_A?&free_a:&free_b,1);
    }else guest_trap("unexpected callback",function);
    memset(c,0xa5,sizeof *c);c->x[0]=result;
}
static void *concurrent(void *arg){
    web_memory_bind(&memory);uint64_t object=BASE+0x2000+(uintptr_t)arg*256,copy=object+64;
    for(unsigned n=0;n<32;n++){
        invoke("_ZN5movie6BufferC1Em",object,6000,0);uint64_t pointer=rd64(object+40);check(pointer!=0&&rd8(object+16)==1&&rd64(object+24)==6000);
        wr32(pointer+5996,n);invoke("_ZN5movie6BufferC2ERKS0_",copy,object,0);
        uint64_t copied=rd64(copy+40);check(copied!=pointer&&rd32(copied+5996)==n);
        invoke("_ZN5movie6BufferD1Ev",object,0,0);invoke("_ZN5movie6BufferD2Ev",copy,0,0);
        uint64_t m=object+128,mc=m+8;
        invoke("_ZN5movie9MediaDataC1Ev",m,0,0);invoke("_ZN5movie9MediaData9SetStringEPKcS2_",m,BASE+0x700,BASE+0x740);
        invoke("_ZN5movie9MediaDataC2ERKS0_",mc,m,0);invoke("_ZN5movie9MediaDataD1Ev",m,0,0);
        check(invoke("_ZNK5movie9MediaData10FindStringEPKcPS2_",mc,BASE+0x700,object+144)==1);
        check(rd8(rd64(object+144))=='v');invoke("_ZN5movie9MediaDataD2Ev",mc,0,0);
    }
    web_memory_bind(NULL);return NULL;
}
const char *movie_values_probe(void){
    gm_init_limit(&memory,512);check(gm_reserve(&memory,BASE,2*1024*1024)==GM_OK);check(gm_commit(&memory,BASE,2*1024*1024)==GM_OK);web_memory_bind(&memory);web_movie_reset();
    const uint64_t a=BASE+0x1000,b=a+64,d=a+128;
    invoke("_ZN5movie6BufferC1EPvm",BASE+0x800,BASE+0x900,100);
    web_write(BASE+0x700,"mime",5);web_write(BASE+0x740,"video/x-vnd.on2.vp9",19);
    invoke("_ZN5movie6BufferC1EPvm",a,BASE+0x4000,512);
    check(rd64(a+40)==BASE+0x4000&&rd64(a+24)==512&&rd32(a+4)==0&&rd8(a+16)==0);
    invoke("_ZN5movie6Buffer8SetRangeEii",a,17,123);
    check(rd32(a)==17&&rd32(a+4)==123&&rd64(a+32)==0&&rd64(a+40)==BASE+0x4000);
    check(invoke("_ZNK5movie6Buffer4DataEv",a,0,0)==BASE+0x4011);
    invoke("_ZN5movie6Buffer12SetInt32DataEi",a,0x12345678,0);invoke("_ZN5movie6Buffer13SetUint32DataEj",a,0xaabbccdd,0);invoke("_ZN5movie6Buffer12SetInt64DataEl",a,UINT64_C(0x1122334455667788),0);
    check(rd32(a+8)==0x12345678&&rd32(a+12)==0xaabbccdd&&rd64(a+32)==UINT64_C(0x1122334455667788));
    check(invoke("_ZNK5movie6Buffer12GetInt64DataEv",a,0,0)==UINT64_C(0x1122334455667788));
    invoke("_ZN5movie6BufferC2ERKS0_",b,a,0);check(rd64(b+40)==rd64(a+40)&&rd8(b+16)==0);invoke("_ZN5movie6BufferD1Ev",b,0,0);
    allocator_set(ALLOC_A,FREE_A,111);
    Ctx c={0};for(unsigned i=0;i<32;i++)c.x[i]=0x123000+i;c.x[0]=b;c.x[1]=6000;c.v[7].d[0]=12345;c.tpidr=0x7800020000;c.fn=0x7100001234;c.N=1;c.C=1;Ctx saved=c;
    check(web_movie_import(&c,"_ZN5movie6BufferC1Em"));check(!memcmp(&c,&saved,sizeof c));
    uint64_t original=rd64(b+40);check(original!=0&&rd8(b+16)==1&&web_movie_live_allocations()==1);
    wr32(original+4094,0x12345678);invoke("_ZN5movie6Buffer8SetRangeEii",b,3,5997);
    allocator_set(ALLOC_B,FREE_B,222);
    invoke("_ZN5movie6BufferC1ERKS0_",d,b,0);uint64_t copied=rd64(d+40);check(copied!=original&&rd32(copied+4094)==0x12345678&&rd32(d)==3&&rd32(d+4)==5997);
    wr32(original+4094,0);check(rd32(copied+4094)==0x12345678);
    invoke("_ZN5movie6Buffer4SwapERS0_",a,b,0);check(rd64(a+40)==original&&rd8(a+16)==1&&rd8(b+16)==0);
    invoke("_ZN5movie6BufferD1Ev",a,0,0);check(atomic_load(&free_a)==1&&web_movie_live_allocations()==1);
    invoke("_ZN5movie6BufferaSERKS0_",d,d,0);check(rd64(d+40)==copied&&web_movie_live_allocations()==1);
    invoke("_ZN5movie6Buffer18SetDataAndCapacityEPvm",d,BASE+0x4000,10);check(rd64(d+40)==BASE+0x4000&&rd8(d+16)==0&&web_movie_live_allocations()==0);
    atomic_store(&fail_allocate,1);invoke("_ZN5movie6BufferC1Em",a,100,0);check(rd64(a+40)==0&&rd8(a+16)==0&&web_movie_live_allocations()==0);invoke("_ZN5movie6BufferD1Ev",a,0,0);atomic_store(&fail_allocate,0);
    uint64_t md=BASE+0x5000,mc=md+8,out=md+16,key=md+32;
    invoke("_ZN5movie9MediaDataC1Ev",md,0,0);check(web_movie_live_metadata()==1);
    web_write(key,"integer",8);wr64(out,0xdeadbeef);
    check(!invoke("_ZNK5movie9MediaData9FindInt32EPKcPi",md,key,out)&&rd64(out)==0xdeadbeef);
    invoke("_ZN5movie9MediaData8SetInt32EPKci",md,key,0xffffffff);
    check(invoke("_ZNK5movie9MediaData9FindInt64EPKcPl",md,key,out)&&rd64(out)==UINT64_MAX);
    invoke("_ZN5movie9MediaData8SetInt64EPKcl",md,key,UINT64_C(0x12345678abcdef01));
    check(invoke("_ZNK5movie9MediaData9FindInt32EPKcPi",md,key,out)&&rd32(out)==0xabcdef01);
    {uint64_t value=0;check(web_movie_metadata_get(md,"integer",WEB_MOVIE_I32,&value)&&value==0xabcdef01);}
    check(!invoke("_ZNK5movie9MediaData10FindStringEPKcPS2_",md,key,out));
    invoke("_ZN5movie9MediaData10SetPointerEPKcPv",md,key,UINT64_C(0x8012345678));
    check(invoke("_ZNK5movie9MediaData11FindPointerEPKcPPv",md,key,out)&&rd64(out)==UINT64_C(0x8012345678));
    check(!invoke("_ZNK5movie9MediaData9FindInt64EPKcPl",md,key,out));
    invoke("_ZN5movie9MediaData9SetStringEPKcS2_",md,BASE+0x700,BASE+0x740);
    check(invoke("_ZNK5movie9MediaData10FindStringEPKcPS2_",md,BASE+0x700,out));uint64_t str=rd64(out);check(str!=BASE+0x740&&rd8(str)=='v'&&web_movie_live_allocations()==1);
    invoke("_ZN5movie9MediaDataC2ERKS0_",mc,md,0);check(web_movie_live_metadata()==2&&web_movie_live_allocations()==2);
    check(invoke("_ZNK5movie9MediaData10FindStringEPKcPS2_",mc,BASE+0x700,out)&&rd64(out)!=str);
    invoke("_ZN5movie9MediaData5ClearEv",md,0,0);check(web_movie_live_allocations()==1&&invoke("_ZNK5movie9MediaData12CountEntriesEv",md,0,0)==0);
    check(invoke("_ZN5movie9MediaDataaSERKS0_",md,mc,0)==md&&web_movie_live_allocations()==2);
    check(invoke("_ZN5movie9MediaDataaSERKS0_",md,md,0)==md&&web_movie_live_allocations()==2);
    invoke("_ZN5movie9MediaData9SetStringEPKcS2_",md,BASE+0x700,0);check(invoke("_ZNK5movie9MediaData10FindStringEPKcPS2_",md,BASE+0x700,out)&&rd8(rd64(out))==0);
    invoke("_ZN5movie9MediaData8SetInt32EPKci",md,BASE+0x700,7);check(web_movie_live_allocations()==1&&invoke("_ZNK5movie9MediaData8ContainsEPKc",md,BASE+0x700,0));
    invoke("_ZN5movie9MediaDataD1Ev",md,0,0);invoke("_ZN5movie9MediaDataD2Ev",mc,0,0);check(!rd64(md)&&!rd64(mc)&&web_movie_live_metadata()==0&&web_movie_live_allocations()==0);
    pthread_t workers[2];for(uintptr_t i=0;i<2;i++)check(pthread_create(&workers[i],NULL,concurrent,(void *)i)==0);for(unsigned i=0;i<2;i++)check(pthread_join(workers[i],NULL)==0);
    check(web_movie_live_allocations()==0&&web_movie_live_metadata()==0);check(atomic_load(&alloc_a)==atomic_load(&free_a));check(atomic_load(&alloc_b)==atomic_load(&free_b)+1); /* one explicit allocation failure */
    web_movie_reset();web_memory_bind(NULL);gm_destroy(&memory);
    snprintf(report,sizeof report,"{\"ok\":%s,\"checks\":%u,\"failures\":%u}",atomic_load(&failures)?"false":"true",atomic_load(&checks),atomic_load(&failures));return report;
}
