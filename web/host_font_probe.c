/* Exercises the production adapter with real GuestMemory and local font files. */
#include "host_font.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
static GuestMemory memory;
static const uint64_t base=UINT64_C(0x8100000000);
static atomic_uint calls;
static uint64_t callback_engine,callback_next;
static unsigned allocations,frees;
void guest_trap(const char *what,uint64_t address) {
    fprintf(stderr,"fixture trap: %s %llx\n",what,(unsigned long long)address); abort();
}
void call_indirect(Ctx *c,uint64_t address) {
    /* Reenter the font adapter during a caller allocator callback: holding the
     * adapter lock across guest execution would deadlock this check. */
    Ctx nested={0}; nested.x[0]=callback_engine;
    assert(web_font_import(&nested,"_ZN2nn6fontll18ScalableFontEngine8GetErrorEv"));
    if (address==0x1234) {
        assert(c->x[1]==4 && c->x[2]==0xabcdef);
        uint64_t size=c->x[0]; c->x[0]=callback_next; callback_next+=(size+7)&~UINT64_C(7); allocations++;
    } else if (address==0x5678) { assert(c->x[0] && c->x[1]==0xabcdef); frees++; }
    else guest_trap("unexpected callback",address);
}
#define F(s) "_ZN2nn6fontll18ScalableFontEngine" s
static uint64_t invoke(const char *name,uint64_t self,uint64_t a,uint64_t b,uint64_t d,uint64_t e,uint64_t f) {
    Ctx c={0}; c.x[0]=self;c.x[1]=a;c.x[2]=b;c.x[3]=d;c.x[4]=e;c.x[5]=f;
    assert(web_font_import(&c,name)); atomic_fetch_add(&calls,1); return c.x[0];
}
static uint64_t shared(unsigned type) {
    return invoke("_ZN2nn2pl20GetSharedFontAddressENS0_14SharedFontTypeE",type,0,0,0,0,0);
}
static uint64_t glyph(uint64_t engine,uint32_t code) { return invoke(F("15AcquireGlyphmapEjt"),engine,code,8,0,0,0); }
static void drop(uint64_t engine,uint64_t map) { assert(!invoke(F("13ReleasesGlyphEPv"),engine,map,0,0,0,0)); }
static void prepare(uint64_t engine,uint64_t font,uint64_t name) {
    assert(invoke(F("C1Ev"),engine,0,0,0,0,0)==engine);
    assert(!invoke(F("10InitializeEPvj"),engine,base+0x90000,4096,0,0,0));
    assert(!invoke(F("8LoadFontEPcPKvjj"),engine,name,font,0,50,0));
    assert(!invoke(F("7SetFontEPc"),engine,name,0,0,0,0));
}
static void *thread_test(void *arg) {
    web_memory_bind(&memory); uint64_t engine=*(const uint64_t*)arg;
    for (unsigned i=0;i<100;i++) {
        uint64_t map=glyph(engine,'A'); assert(map && rd16(map+28)>0 && rd16(map+30)>0);
        uint32_t n=rd32(map+8); assert(n==40+(uint32_t)rd16(map+28)*rd16(map+30));
        drop(engine,map);
    }
    return NULL;
}
int main(void) {
    assert(gm_init_limit(&memory,GM_HARD_MAX_PAGES)==GM_OK);
    assert(gm_reserve(&memory,base,64u<<20)==GM_OK);
    assert(gm_commit(&memory,base,64u<<20)==GM_OK);
    web_memory_bind(&memory); web_font_reset();
    uint64_t names[7];
    for (unsigned i=0;i<7;i++) {
        uint64_t address=shared(i); assert(address>UINT32_MAX && address==shared(i));
        uint64_t size=invoke("_ZN2nn2pl17GetSharedFontSizeENS0_14SharedFontTypeE",i,0,0,0,0,0);
        assert(size>100000 && size<10u*1024u*1024u);
        assert(invoke("_ZN2nn2pl22GetSharedFontLoadStateENS0_14SharedFontTypeE",i,0,0,0,0,0)==1);
        assert(rd8(address)==0 && rd8(address+1)==1);
        names[i]=base+0x10000+i*4096+4090;
        prepare(base+0x1000+i*512,address,names[i]);
        for (unsigned j=0;j<i;j++) {
            char a[50],b[50]; web_read(names[i],a,50);web_read(names[j],b,50); assert(strcmp(a,b));
        }
        assert(!invoke(F("14GetFontMetricsEPNS0_7MetricsE"),base+0x1000+i*512,base+0x80000+4090,0,0,0,0));
        assert(rd16(base+0x80000+4090)==(i==3 ? 1000 : 1024));
        assert(!invoke(F("25InitializeOtfKerningTableEPFPvmmS2_ES2_b"),base+0x1000+i*512,0x1234,0,0,0,0));
    }
    uint64_t a=base+0xff0,b=base+0x2ff0;
    prepare(a,shared(0),base+0x50000+4091); prepare(b,shared(0),base+0x51000+4091);
    char n0[50],n1[50]; web_read(base+0x50000+4091,n0,50); web_read(base+0x51000+4091,n1,50); assert(!strcmp(n0,n1));
    uint64_t ga=glyph(a,'A'),gb=glyph(b,'B'); assert(ga && gb && ga!=gb);
    size_t na=rd32(ga+8); uint8_t *saved=malloc(na),*after=malloc(na); assert(saved && after);
    web_read(ga,saved,na); drop(b,gb); web_read(ga,after,na); assert(!memcmp(saved,after,na));
    drop(a,ga); assert(gm_read(&memory,ga,after,1)==GM_UNMAPPED); free(saved);free(after);
    uint64_t reused=glyph(a,'A'); assert(reused==ga); drop(a,reused);
    uint64_t space=glyph(a,' '); assert(space && rd32(space+8)==40 && !rd16(space+28)); drop(a,space);
    assert(!invoke(F("15CheckGlyphExistEj"),a,0x10ffff,0,0,0,0));
    assert(invoke(F("15SetOutlineWidthEt"),a,11,0,0,0,0)==0x266);
    assert(invoke(F("8GetErrorEv"),a,0,0,0,0,0)==0x266);
    assert(!invoke(F("15SetOutlineWidthEt"),a,2,0,0,0,0));
    assert(invoke(F("8SetFlagsENS1_5FlagsE"),a,3,0,0,0,0)==0x264);
    assert(invoke(F("7SetFontEPc"),a,base+0x60000,0,0,0,0)==0x12e);
    assert(invoke(F("8LoadFontEPcPKvjj"),a,base+0x60000,base+0x70000,0,50,0)==0x12e);
    assert(!invoke(F("10GetAdvanceEPsS2_PiS3_jt"),a,base+0x61000,base+0x61002,base+0x61004,base+0x61008,'A'));
    assert(rd32(base+0x61004)>0 && !rd32(base+0x61008));
    uint8_t original[20],encoded[20],decoded[20];
    static const uint8_t key[]={0x49,0x62,0x18,0x06};
    for (unsigned i=0;i<20;i++) { original[i]=(uint8_t)(i*7); encoded[i]=original[i]^key[i&3]; }
    web_write(base+0x70ff8,encoded,sizeof encoded);
    assert(invoke("_ZN2nn6fontll24ScalableFontEngineHelper6DecodeEPKvj",base+0x70ff8,20,0,0,0,0)==base+0x71000);
    web_read(base+0x70ff8,decoded,sizeof decoded); assert(!memcmp(decoded,original,sizeof original));
    size_t pages=memory.page_count;
    for (unsigned i=0;i<500;i++) { uint64_t map=glyph(a,'A'+i%26); assert(map); drop(a,map); }
    assert(memory.page_count==pages);
    FILE *fixture=fopen("/fixture-gpos.ttf","rb"); assert(fixture);
    assert(!fseek(fixture,0,SEEK_END)); long length=ftell(fixture); rewind(fixture);
    assert(length>0); uint8_t *input=malloc((size_t)length); assert(input);
    assert(fread(input,1,(size_t)length,fixture)==(size_t)length); fclose(fixture);
    web_write(base+0x2000000,input,(size_t)length); free(input);
    uint64_t otf_engine=base+0x400ff0;
    prepare(otf_engine,base+0x2000000,base+0x500ff0);
    callback_engine=otf_engine; callback_next=base+0x1000ff8;
    uint64_t table=invoke(F("25InitializeOtfKerningTableEPFPvmmS2_ES2_b"),otf_engine,0x1234,0xabcdef,0,0,0);
    assert(table && rd32(table)==2 && rd64(table+8)>UINT32_MAX && allocations==2);
    assert((int32_t)invoke(F("17AcquireOtfKerningEPKNS0_15OtfKerningTableEjjj"),otf_engine,table,'A','B',1024,0)==-65);
    assert(invoke(F("22AcquireOtfKerningFirstEPKNS0_15OtfKerningTableEjj"),otf_engine,table,'A',1024,0,0)==5);
    assert(invoke(F("21AcquireOtfKerningLastEPKNS0_15OtfKerningTableEjj"),otf_engine,table,'A',1024,0,0)==15);
    assert(!invoke(F("23FinalizeOtfKerningTableEPNS0_15OtfKerningTableEPFvPvS4_ES4_"),otf_engine,table,0x5678,0xabcdef,0,0) && frees==2);
    table=invoke(F("25InitializeOtfKerningTableEPFPvmmS2_ES2_b"),otf_engine,0x1234,0xabcdef,1,0,0);
    assert(table && rd32(table)==1 && allocations==4);
    assert((int32_t)invoke(F("17AcquireOtfKerningEPKNS0_15OtfKerningTableEjjj"),otf_engine,table,'A','B',1024,0)==-80);
    assert(!invoke(F("23FinalizeOtfKerningTableEPNS0_15OtfKerningTableEPFvPvS4_ES4_"),otf_engine,table,0x5678,0xabcdef,0,0) && frees==4);
    /* Two host threads acquire/release independent glyphs on separate engines. */
    pthread_t threads[2];
    assert(!pthread_create(&threads[0],NULL,thread_test,&a));
    assert(!pthread_create(&threads[1],NULL,thread_test,&b));
    assert(!pthread_join(threads[0],NULL) && !pthread_join(threads[1],NULL));
    assert(memory.page_count==pages);
    assert(!invoke(F("8FinalizeEv"),a,0,0,0,0,0));
    assert(invoke(F("C1Ev"),a,0,0,0,0,0)==a);
    assert(!invoke(F("15CheckGlyphExistEj"),a,'A',0,0,0,0));
    assert(invoke(F("8GetErrorEv"),a,0,0,0,0,0)==0x131);
    web_font_reset(); gm_destroy(&memory);
    printf("PASS: real shared fonts, cross-page guest ABI, glyph ownership/reuse, errors, 700 glyph cycles, concurrent threads (%u imports)\n",atomic_load(&calls));
    return 0;
}
