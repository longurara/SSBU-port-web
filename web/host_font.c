/* Guest-safe nn::pl/nn::fontll adapter. Font files remain user-supplied local
 * inputs mounted at /fonts; host pointers are never returned to guest code. */
#include "host_font.h"
#include "font_raster.h"
#include "font_otf.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FONT_BASE UINT64_C(0x7c00000000)
#define ARENA_PAGES 32768u
#define MAX_ENGINES 256
#define MAX_FONTS 64
#define FONT_LIMIT (32u*1024u*1024u)
#define ERR_FONT 0x12e
#define ERR_NONE 0x131
#define ERR_MEMORY 0xc9
typedef struct { WebFont *font; uint8_t *bytes; size_t size; unsigned face,hash; char name[50]; } Font;
typedef struct {
    uint64_t key,work; Font *fonts[32]; char names[32][50]; unsigned count; int current,error,bold;
    WebFontStyle style;
} Engine;
typedef struct Glyph { uint64_t address,owner; size_t pages; struct Glyph *next; } Glyph;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static Font fonts[MAX_FONTS]; static unsigned font_count;
static Engine engines[MAX_ENGINES];
static uint8_t occupied[ARENA_PAGES]; static int reserved;
static Glyph *glyphs;
static struct { uint64_t address; size_t size; } shared[7];
static const char *const filenames[7]={
    "nintendo_udsg-r_std_003.ttf","nintendo_udsg-r_org_zh-cn_003.ttf",
    "nintendo_udsg-r_ext_zh-cn_003.ttf","nintendo_udjxh-db_zh-tw_003.ttf",
    "nintendo_udsg-r_ko_003.ttf","nintendo_ext_003.ttf","nintendo_ext2_003.ttf"};
static unsigned be16(const uint8_t *p) { return (unsigned)p[0]*256+p[1]; }
static unsigned be32(const uint8_t *p) { return (be16(p)<<16)|be16(p+2); }
static uint64_t allocate(size_t size) {
    GuestMemory *m=web_memory_current(); if (!m || !size || size>FONT_LIMIT) return 0;
    if (!reserved) {
        if (gm_reserve(m,FONT_BASE,(uint64_t)ARENA_PAGES*GM_PAGE_SIZE)!=GM_OK) return 0;
        reserved=1;
    }
    size_t n=(size+GM_PAGE_SIZE-1)/GM_PAGE_SIZE,run=0;
    for (size_t i=0;i<ARENA_PAGES;i++) {
        run=occupied[i] ? 0 : run+1;
        if (run==n) {
            size_t first=i+1-n; uint64_t address=FONT_BASE+first*GM_PAGE_SIZE;
            if (gm_commit(m,address,n*GM_PAGE_SIZE)!=GM_OK) return 0;
            memset(occupied+first,1,n); return address;
        }
    }
    return 0;
}
static void release(uint64_t address,size_t pages) {
    if (gm_decommit(web_memory_current(),address,pages*GM_PAGE_SIZE)==GM_OK)
        memset(occupied+(size_t)((address-FONT_BASE)/GM_PAGE_SIZE),0,pages);
}
void web_font_reset(void) {
    pthread_mutex_lock(&lock);
    for (unsigned i=0;i<font_count;i++) { web_font_close(fonts[i].font); free(fonts[i].bytes); }
    while (glyphs) { Glyph *next=glyphs->next; free(glyphs); glyphs=next; }
    memset(fonts,0,sizeof fonts); memset(engines,0,sizeof engines); memset(shared,0,sizeof shared);
    memset(occupied,0,sizeof occupied); font_count=0; reserved=0;
    pthread_mutex_unlock(&lock);
}
static Engine *engine(uint64_t key) {
    if (!key) return NULL;
    Engine *empty=NULL;
    for (unsigned i=0;i<MAX_ENGINES;i++) {
        if (engines[i].key==key) return &engines[i];
        if (!engines[i].key && !empty) empty=&engines[i];
    }
    if (empty) {
        *empty=(Engine){.key=key,.current=-1,.style={{32<<16,0,0,32<<16},1,0}};
        wr64(key,(uint64_t)(empty-engines)); wr64(key+8,UINT64_C(0x6c6c746e6f666e6e));
    }
    return empty;
}
static Font *current(Engine *e) { return e->current>=0 ? e->fonts[e->current] : NULL; }
static int read_at(uint64_t base,size_t offset,void *out,size_t size) {
    if (offset>FONT_LIMIT || size>FONT_LIMIT-offset || base>UINT64_MAX-offset-size) return 0;
    return gm_read(web_memory_current(),base+offset,out,size)==GM_OK;
}
/* LoadFont has no size argument. Derive the bounded SFNT/TTC extent from table
 * directories, then copy through gm_read (pages are not contiguous host spans). */
static uint8_t *copy_font(uint64_t address,size_t *size) {
    uint8_t header[12]; if (!read_at(address,0,header,sizeof header)) return NULL;
    unsigned faces=1; int collection=!memcmp(header,"ttcf",4);
    if (collection) faces=be32(header+8);
    if (!faces || faces>64) return NULL;
    size_t end=collection ? 12+(size_t)faces*4 : 12;
    for (unsigned face=0;face<faces;face++) {
        uint8_t word[4]; size_t offset=0;
        if (collection) {
            if (!read_at(address,12+face*4,word,4)) return NULL;
            offset=be32(word);
        }
        if (!read_at(address,offset,header,12)) return NULL;
        if (memcmp(header,"OTTO",4) && be32(header)!=0x10000 && memcmp(header,"true",4)) return NULL;
        unsigned tables=be16(header+4); if (!tables || tables>256) return NULL;
        size_t directory=offset+12+(size_t)tables*16;
        if (directory>FONT_LIMIT) return NULL;
        if (directory>end) end=directory;
        uint8_t records[256*16];
        if (!read_at(address,offset+12,records,(size_t)tables*16)) return NULL;
        for (unsigned i=0;i<tables;i++) {
            size_t start=be32(records+i*16+8),length=be32(records+i*16+12);
            if (start>FONT_LIMIT || length>FONT_LIMIT-start) return NULL;
            if (start+length>end) end=start+length;
        }
    }
    end=(end+3)&~(size_t)3;
    uint8_t *bytes=malloc(end); if (!bytes) return NULL;
    if (!read_at(address,0,bytes,end)) { free(bytes); return NULL; }
    *size=end; return bytes;
}
static Font *load(uint64_t address,unsigned face) {
    size_t size=0; uint8_t *bytes=copy_font(address,&size); if (!bytes) return NULL;
    unsigned hash=2166136261u;
    for (size_t i=0;i<size;i++) hash=(hash^bytes[i])*16777619u;
    for (unsigned i=0;i<font_count;i++) {
        Font *f=&fonts[i];
        if (f->hash==hash && f->face==face && f->size==size && !memcmp(f->bytes,bytes,size)) {
            free(bytes); return f;
        }
    }
    if (font_count==MAX_FONTS) { free(bytes); return NULL; }
    WebFont *raster=NULL;
    if (web_font_open(bytes,size,face,&raster)) { free(bytes); return NULL; }
    Font *f=&fonts[font_count++];
    *f=(Font){.font=raster,.bytes=bytes,.size=size,.face=face,.hash=hash};
    /* Cache identity is shared by engines, never a per-engine Font0 alias. */
    snprintf(f->name,sizeof f->name,"Font%u",font_count-1);
    return f;
}
static int shared_load(unsigned type) {
    if (type>=7) return 0;
    if (shared[type].address) return 1;
    char path[128]; snprintf(path,sizeof path,"/fonts/%s",filenames[type]);
    FILE *file=fopen(path,"rb"); if (!file) return 0;
    if (fseek(file,0,SEEK_END)) { fclose(file); return 0; }
    long length=ftell(file); rewind(file);
    if (length<=0 || (unsigned long)length>FONT_LIMIT) { fclose(file); return 0; }
    uint8_t *bytes=malloc((size_t)length); if (!bytes) { fclose(file); return 0; }
    int ok=fread(bytes,1,(size_t)length,file)==(size_t)length; fclose(file);
    WebFont *check=NULL;
    if (ok) ok=!web_font_open(bytes,(size_t)length,0,&check);
    web_font_close(check);
    uint64_t address=ok ? allocate((size_t)length) : 0;
    if (address) web_write(address,bytes,(size_t)length);
    free(bytes);
    if (!address) return 0;
    shared[type].address=address; shared[type].size=(size_t)length; return 1;
}
static void callback(Ctx *c,uint64_t function) { call_indirect(c,function); }
static int pair_find(uint64_t entries,unsigned count,unsigned first,unsigned second,int *adv,int *plc) {
    unsigned lo=0,hi=count;
    while (lo<hi) {
        unsigned mid=lo+(hi-lo)/2; uint64_t a=entries+(uint64_t)mid*8;
        unsigned g1=rd16(a),g2=rd16(a+2);
        if (g1<first || (g1==first && g2<second)) lo=mid+1; else hi=mid;
    }
    if (lo>=count) return 0;
    uint64_t a=entries+(uint64_t)lo*8;
    if (rd16(a)!=first || rd16(a+2)!=second) return 0;
    *adv=(int16_t)rd16(a+4); *plc=(int16_t)rd16(a+6); return 1;
}
int web_font_import(Ctx *c,const char *name) {
    const char *m=NULL;
    const char *prefix="_ZN2nn6fontll18ScalableFontEngine";
    if (!strncmp(name,prefix,strlen(prefix))) m=name+strlen(prefix);
    int pl=!strcmp(name,"_ZN2nn2pl21RequestSharedFontLoadENS0_14SharedFontTypeE") ? 1 :
        !strcmp(name,"_ZN2nn2pl22GetSharedFontLoadStateENS0_14SharedFontTypeE") ? 2 :
        !strcmp(name,"_ZN2nn2pl20GetSharedFontAddressENS0_14SharedFontTypeE") ? 3 :
        !strcmp(name,"_ZN2nn2pl17GetSharedFontSizeENS0_14SharedFontTypeE") ? 4 : 0;
    int decode=!strcmp(name,"_ZN2nn6fontll24ScalableFontEngineHelper6DecodeEPKvj");
    if (!m && !pl && !decode) return 0;
    pthread_mutex_lock(&lock);
    if (pl) {
        unsigned type=(unsigned)c->x[0]; int ok=shared_load(type);
        uint64_t result=ok ? (pl==3 ? shared[type].address : pl==4 ? shared[type].size : pl==2 ? 1 : 0) : 0;
        pthread_mutex_unlock(&lock);
        if (!ok && type<7) guest_trap("required system font missing or invalid in /fonts",type);
        c->x[0]=result; return 1;
    }
    if (decode) {
        uint64_t address=c->x[0]; uint32_t size=(uint32_t)c->x[1]&~3u;
        if (!address || size<12 || size>FONT_LIMIT) c->x[0]=0;
        else {
            uint8_t block[4096]; static const uint8_t key[]={0x49,0x62,0x18,0x06};
            for (size_t at=0;at<size;at+=sizeof block) {
                size_t n=size-at; if (n>sizeof block) n=sizeof block;
                web_read(address+at,block,n);
                for (size_t i=0;i<n;i++) block[i]^=key[i&3];
                web_write(address+at,block,n);
            }
            c->x[0]=address+8;
        }
        pthread_mutex_unlock(&lock); return 1;
    }
    uint64_t self=c->x[0];
    if (!strcmp(m,"C1Ev") || !strcmp(m,"C2Ev")) {
        for (unsigned i=0;i<MAX_ENGINES;i++) if (engines[i].key==self) engines[i].key=0;
        uint8_t empty[0xb8]={0}; web_write(self,empty,sizeof empty);
        if (!engine(self)) { pthread_mutex_unlock(&lock); guest_trap("font engine registry exhausted",self); }
        c->x[0]=self; pthread_mutex_unlock(&lock); return 1;
    }
    Engine *e=engine(self);
    if (!e) { pthread_mutex_unlock(&lock); guest_trap("invalid font engine",self); }
    Font *f=current(e); uint64_t result=0;
    if (!strcmp(m,"10InitializeEPvj")) { e->work=c->x[1]; wr64(self+0xb0,e->work); e->error=0; }
    else if (!strcmp(m,"8FinalizeEv")) { memset(e,0,sizeof *e); wr64(self,0); wr64(self+8,0); }
    else if (!strcmp(m,"8GetErrorEv")) result=(unsigned)e->error;
    else if (!strcmp(m,"22GetPointerToWorkBufferEv")) result=e->work;
    else if (!strcmp(m,"8LoadFontEPcPKvjj")) {
        Font *loaded=c->x[2] ? load(c->x[2],(unsigned)c->x[3]) : NULL;
        e->error=ERR_FONT;
        if (loaded && e->count<32) {
            char output[50]={0}; size_t room=(unsigned)c->x[4];
            if (!room || room>sizeof output) room=sizeof output;
            snprintf(output,room,"%s",loaded->name);
            e->fonts[e->count]=loaded; memcpy(e->names[e->count],output,sizeof output); e->count++;
            if (c->x[1]) web_write(c->x[1],output,room);
            e->error=0;
        }
        result=(unsigned)e->error;
    } else if (!strcmp(m,"7SetFontEPc")) {
        char selected[50]={0};
        for (unsigned i=0;i<49;i++) { selected[i]=(char)rd8(c->x[1]+i); if (!selected[i]) break; }
        e->error=ERR_FONT;
        for (unsigned i=0;i<e->count;i++) if (!strcmp(selected,e->names[i])) { e->current=(int)i; e->error=0; break; }
        result=(unsigned)e->error;
    } else if (!strcmp(m,"8SetScaleEiiii")) {
        for (unsigned i=0;i<4;i++) e->style.scale[i]=(int32_t)c->x[i+1];
        result=e->error=f ? 0 : ERR_NONE;
    } else if (!strcmp(m,"13SetBoldWeightEi")) {
        result=e->error=(unsigned)c->x[1]>0x10000 ? 0x265 : 0;
        if (!e->error) e->bold=(int)c->x[1]; /* Same known limitation as native. */
    } else if (!strcmp(m,"15SetOutlineWidthEt")) {
        unsigned n=(unsigned)c->x[1]&0xffff; result=e->error=n<1 || n>10 ? 0x266 : 0;
        if (!e->error) e->style.outline=n;
    } else if (!strcmp(m,"8SetFlagsENS1_5FlagsE")) {
        unsigned mode=(unsigned)c->x[1]; result=e->error=mode>2 ? 0x264 : 0;
        if (!e->error) e->style.effect=mode;
    } else if (!strcmp(m,"11SetAutoHintEb")) e->error=0;
    else if (!strcmp(m,"14GetFontMetricsEPNS0_7MetricsE")) {
        uint8_t metrics[0x93c]={0}; e->error=f ? (web_font_metrics(f->font,metrics) ? ERR_FONT : 0) : ERR_NONE;
        if (c->x[1]) web_write(c->x[1],metrics,sizeof metrics); result=(unsigned)e->error;
    } else if (!strcmp(m,"15CheckGlyphExistEj")) {
        result=f && web_font_glyph(f->font,(uint32_t)c->x[1]); e->error=f ? 0 : ERR_NONE;
    } else if (!strcmp(m,"10GetAdvanceEPsS2_PiS3_jt") || !strcmp(m,"10GetKerningEPiS2_jj")) {
        int32_t dx=0,dy=0; int kern=!strcmp(m,"10GetKerningEPiS2_jj");
        e->error=f ? ((kern ? web_font_kerning(f->font,(uint32_t)c->x[3],(uint32_t)c->x[4],&e->style,&dx,&dy) :
                               web_font_advance(f->font,(uint32_t)c->x[5],&e->style,&dx,&dy)) ? ERR_FONT : 0) : ERR_NONE;
        if (kern) { if (c->x[1]) wr32(c->x[1],(uint32_t)dx); if (c->x[2]) wr32(c->x[2],(uint32_t)dy); }
        else {
            if (c->x[1]) wr16(c->x[1],(uint16_t)((dx+0x8000)>>16)); if (c->x[2]) wr16(c->x[2],(uint16_t)((dy+0x8000)>>16));
            if (c->x[3]) wr32(c->x[3],(uint32_t)dx); if (c->x[4]) wr32(c->x[4],(uint32_t)dy);
        }
        result=(unsigned)e->error;
    } else if (!strcmp(m,"15AcquireGlyphmapEjt")) {
        uint8_t *map=NULL; size_t size=0;
        e->error=f ? (web_font_raster(f->font,(uint32_t)c->x[1],(unsigned)c->x[2]&0xffff,&e->style,&map,&size) ? ERR_MEMORY : 0) : ERR_NONE;
        if (map) {
            Glyph *g=malloc(sizeof *g); uint64_t address=g ? allocate(size) : 0;
            if (address) {
                web_write(address,map,size); *g=(Glyph){address,self,(size+GM_PAGE_SIZE-1)/GM_PAGE_SIZE,glyphs}; glyphs=g; result=address;
            } else { free(g); e->error=ERR_MEMORY; }
            free(map);
        }
    } else if (!strcmp(m,"13ReleasesGlyphEPv")) {
        Glyph **at=&glyphs;
        while (*at && ((*at)->address!=c->x[1] || (*at)->owner!=self)) at=&(*at)->next;
        if (*at) { Glyph *g=*at; release(g->address,g->pages); *at=g->next; free(g); }
        e->error=0;
    } else if (!strcmp(m,"25InitializeOtfKerningTableEPFPvmmS2_ES2_b")) {
        WebFontPair *pairs=NULL; size_t count=0; size_t table_size=0;
        if (f && !web_font_table(f->font,"GPOS",NULL,&table_size) && c->x[1]) {
            int error=web_font_otf(f->font,(int)(c->x[3]&1),&pairs,&count);
            uint64_t fn=c->x[1],user=c->x[2]; float per=1.0f/web_font_units(f->font);
            pthread_mutex_unlock(&lock);
            if (!error) {
                c->x[0]=0x18; c->x[1]=4; c->x[2]=user; callback(c,fn);
                uint64_t table=c->x[0],entries=0;
                if (table) {
                    if (count) { c->x[0]=count*8; c->x[1]=4; c->x[2]=user; callback(c,fn); entries=c->x[0]; }
                    uint8_t empty[0x18]={0}; web_write(table,empty,sizeof empty);
                    wr32(table,entries ? (uint32_t)count : 0); wr64(table+8,entries); web_write(table+16,&per,4);
                    if (entries) web_write(entries,pairs,count*8);
                }
                result=table;
            }
            free(pairs); c->x[0]=result; return 1;
        }
    } else if (!strcmp(m,"23FinalizeOtfKerningTableEPNS0_15OtfKerningTableEPFvPvS4_ES4_")) {
        uint64_t table=c->x[1],fn=c->x[2],user=c->x[3],entries=table ? rd64(table+8) : 0;
        pthread_mutex_unlock(&lock);
        if (table && fn) {
            if (entries) { c->x[0]=entries; c->x[1]=user; callback(c,fn); }
            c->x[0]=table; c->x[1]=user; callback(c,fn);
        }
        c->x[0]=0; return 1;
    } else if (!strcmp(m,"17AcquireOtfKerningEPKNS0_15OtfKerningTableEjjj") ||
               !strcmp(m,"21AcquireOtfKerningLastEPKNS0_15OtfKerningTableEjj") ||
               !strcmp(m,"22AcquireOtfKerningFirstEPKNS0_15OtfKerningTableEjj")) {
        uint64_t table=c->x[1];
        if (f && table) {
            unsigned n=rd32(table); uint64_t entries=rd64(table+8); float per; web_read(table+16,&per,4);
            int a=0,ap=0,b=0,bp=0,p=0,pp=0,units=0; unsigned size=(unsigned)c->x[3];
            unsigned first=web_font_glyph(f->font,(uint32_t)c->x[2]);
            if (n<=2u*1024u*1024u && entries) {
                pair_find(entries,n,first,0xffff,&a,&ap);
                if (m[0]=='1') {
                    unsigned second=web_font_glyph(f->font,(uint32_t)c->x[3]);
                    pair_find(entries,n,second,0xffff,&b,&bp); pair_find(entries,n,first,second,&p,&pp);
                    units=(int16_t)(a-ap+bp+p); size=(unsigned)c->x[4];
                } else units=m[1]=='1' ? (int16_t)(a-ap) : ap;
                float value=floorf((float)units*per*(float)size+0.5f);
                if (isfinite(value) && value>=INT32_MIN && (double)value<=INT32_MAX) result=(uint32_t)(int32_t)value;
            }
        }
    } else { pthread_mutex_unlock(&lock); return 0; }
    c->x[0]=result; pthread_mutex_unlock(&lock); return 1;
}
