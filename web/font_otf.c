/* SDK OtfKerningTable extraction, following runtime/nnfontll.c. All offsets
 * are checked against the copied GPOS table before reading, including nested
 * coverage/class/extension lookups. No guest addresses or callbacks here. */
#include "font_otf.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t *data; size_t size;
    WebFontPair *pairs; size_t count, capacity, reads;
    int bad;
} Parse;
static int span(Parse *p, size_t at, size_t n) {
    if (p->bad) return 0;
    if (++p->reads>16u*1024u*1024u) { p->bad=1; return 0; }
    if (at > p->size || n > p->size-at) { p->bad=1; return 0; }
    return 1;
}
static unsigned u16(Parse *p, size_t at) {
    return span(p,at,2) ? (unsigned)p->data[at]*256+p->data[at+1] : 0;
}
static unsigned u32(Parse *p, size_t at) {
    return (u16(p,at)<<16)|u16(p,at+2);
}
static void add(Parse *p, unsigned a, unsigned b, int adv, int plc) {
    if (p->bad || (!adv && !plc)) return;
    if (p->count == p->capacity) {
        size_t next = p->capacity ? p->capacity*2 : 1024;
        /* Bound malformed expansion; ordinary system fonts are far smaller. */
        if (next > 2u*1024u*1024u) { p->bad=1; return; }
        void *grown = realloc(p->pairs,next*sizeof *p->pairs);
        if (!grown) { p->bad=1; return; }
        p->pairs=grown; p->capacity=next;
    }
    p->pairs[p->count++]=(WebFontPair){(uint16_t)a,(uint16_t)b,(int16_t)adv,(int16_t)plc};
}
static unsigned value_size(unsigned fmt) {
    unsigned n=0; for (;fmt;fmt>>=1) n+=(fmt&1)*2; return n;
}
static void value(Parse *p,size_t at,unsigned fmt,int *adv,int *plc) {
    *adv=*plc=0;
    if (fmt & ~255u) { p->bad=1; return; }
    if (!span(p,at,value_size(fmt))) return;
    if (fmt&1) { *plc=(int16_t)u16(p,at); at+=2; }
    if (fmt&2) at+=2;
    if (fmt&4) *adv=(int16_t)u16(p,at);
}
typedef void (*Cover)(Parse*,void*,unsigned,unsigned);
static void coverage(Parse *p,size_t at,Cover fn,void *ctx) {
    unsigned fmt=u16(p,at),n=u16(p,at+2);
    if (fmt==1) {
        if (!span(p,at+4,(size_t)n*2)) return;
        for (unsigned i=0;i<n && !p->bad;i++) fn(p,ctx,u16(p,at+4+i*2),i);
    } else if (fmt==2) {
        if (!span(p,at+4,(size_t)n*6)) return;
        unsigned previous=0;
        for (unsigned i=0;i<n && !p->bad;i++) {
            size_t r=at+4+i*6; unsigned first=u16(p,r),last=u16(p,r+2),index=u16(p,r+4);
            if (last<first || (i && first<=previous)) { p->bad=1; break; }
            previous=last;
            for (unsigned g=first;g<=last && !p->bad;g++) fn(p,ctx,g,index+g-first);
        }
    } else p->bad=1;
}
static unsigned class_of(Parse *p,size_t at,unsigned glyph) {
    unsigned fmt=u16(p,at);
    if (fmt==1) {
        unsigned first=u16(p,at+2),n=u16(p,at+4);
        if (!span(p,at+6,(size_t)n*2)) return 0;
        return glyph>=first && glyph-first<n ? u16(p,at+6+(glyph-first)*2) : 0;
    }
    if (fmt==2) {
        unsigned n=u16(p,at+2);
        if (!span(p,at+4,(size_t)n*6)) return 0;
        for (unsigned i=0;i<n;i++) {
            size_t r=at+4+i*6;
            if (glyph>=u16(p,r) && glyph<=u16(p,r+2)) return u16(p,r+4);
        }
        return 0;
    }
    p->bad=1; return 0;
}
typedef struct { size_t sub; unsigned glyphs; uint16_t *seconds; unsigned nsecond; } Context;
static void pair(Parse *p,void *opaque,unsigned glyph,unsigned index) {
    Context *c=opaque; size_t s=c->sub;
    unsigned fmt=u16(p,s),f1=u16(p,s+4),f2=u16(p,s+6);
    if ((f1|f2)&~255u) { p->bad=1; return; }
    if (fmt==1) {
        unsigned n=u16(p,s+8); if (index>=n) return;
        if (!span(p,s+10,(size_t)n*2)) return;
        size_t set=s+u16(p,s+10+index*2); unsigned count=u16(p,set);
        unsigned stride=2+value_size(f1)+value_size(f2);
        if (!span(p,set+2,(size_t)count*stride)) return;
        for (unsigned i=0;i<count;i++) {
            size_t r=set+2+(size_t)i*stride; int adv,plc;
            value(p,r+2,f1,&adv,&plc); add(p,glyph,u16(p,r),adv,plc);
        }
    } else if (fmt==2) {
        unsigned c1=class_of(p,s+u16(p,s+8),glyph),n1=u16(p,s+12),n2=u16(p,s+14);
        unsigned stride=value_size(f1)+value_size(f2);
        if (c1>=n1 || !span(p,s+16,(size_t)n1*n2*stride)) return;
        for (unsigned i=0;i<c->nsecond && !p->bad;i++) {
            unsigned second=c->seconds[i*2],c2=c->seconds[i*2+1];
            if (c2>=n2) continue;
            int adv,plc; value(p,s+16+((size_t)c1*n2+c2)*stride,f1,&adv,&plc);
            add(p,glyph,second,adv,plc);
        }
    } else p->bad=1;
}
static void single(Parse *p,void *opaque,unsigned glyph,unsigned index) {
    Context *c=opaque; size_t s=c->sub; unsigned fmt=u16(p,s),vf=u16(p,s+4);
    int adv,plc;
    if (fmt==1) value(p,s+6,vf,&adv,&plc);
    else if (fmt==2 && index<u16(p,s+6)) value(p,s+8+(size_t)index*value_size(vf),vf,&adv,&plc);
    else { p->bad=1; return; }
    add(p,glyph,0xffff,adv,plc);
}
static size_t feature(Parse *p,const char tag[4]) {
    size_t scripts=u16(p,4),features=u16(p,6);
    unsigned n=u16(p,scripts),nf=u16(p,features);
    if (!span(p,scripts+2,(size_t)n*6) || !span(p,features+2,(size_t)nf*6)) return 0;
    for (unsigned i=0;i<n;i++) {
        size_t r=scripts+2+i*6;
        if (memcmp(p->data+r,"latn",4) && memcmp(p->data+r,"kana",4) && memcmp(p->data+r,"DFLT",4)) continue;
        size_t script=scripts+u16(p,r+4); unsigned off=u16(p,script);
        if (!off) { if (!u16(p,script+2)) continue; off=u16(p,script+8); }
        size_t lang=script+off; unsigned count=u16(p,lang+4);
        if (!span(p,lang+6,(size_t)count*2)) return 0;
        for (unsigned k=0;k<count;k++) {
            unsigned index=u16(p,lang+6+k*2);
            if (index>=nf) { p->bad=1; return 0; }
            size_t rec=features+2+index*6;
            if (!memcmp(p->data+rec,tag,4)) return features+u16(p,rec+4);
        }
        return 0;
    }
    return 0;
}
static int compare(const void *a,const void *b) {
    const WebFontPair *x=a,*y=b;
    return x->first!=y->first ? (int)x->first-y->first : (int)x->second-y->second;
}
int web_font_otf(WebFont *font,int kern_only,WebFontPair **pairs,size_t *count) {
    if (!font || !pairs || !count) return 1;
    *pairs=NULL; *count=0;
    Parse p={0};
    if (web_font_table(font,"GPOS",NULL,&p.size)) return 0;
    if (p.size<10 || p.size>32u*1024u*1024u) return 1;
    p.data=malloc(p.size); if (!p.data) return 1;
    if (web_font_table(font,"GPOS",p.data,&p.size)) { free(p.data); return 1; }
    size_t lookups=u16(&p,8); unsigned nl=u16(&p,lookups);
    if (!span(&p,lookups+2,(size_t)nl*2)) goto done;
    for (unsigned pass=0;pass<(kern_only ? 1u : 2u) && !p.bad;pass++) {
        size_t f=feature(&p,pass ? "palt" : "kern"); if (!f) continue;
        unsigned n=u16(&p,f+2); if (!span(&p,f+4,(size_t)n*2)) break;
        for (unsigned i=0;i<n && !p.bad;i++) {
            unsigned index=u16(&p,f+4+i*2); if (index>=nl) { p.bad=1; break; }
            size_t lookup=lookups+u16(&p,lookups+2+index*2);
            unsigned type=u16(&p,lookup),ns=u16(&p,lookup+4);
            if (!span(&p,lookup+6,(size_t)ns*2)) break;
            for (unsigned k=0;k<ns && !p.bad;k++) {
                size_t s=lookup+u16(&p,lookup+6+k*2); unsigned actual=type;
                if (type==9) {
                    if (u16(&p,s)!=1) { p.bad=1; break; }
                    actual=u16(&p,s+2); unsigned offset=u32(&p,s+4);
                    if (s>p.size || offset>p.size-s) { p.bad=1; break; }
                    s+=offset;
                }
                if (actual!=(pass ? 1u : 2u)) continue;
                Context ctx={.sub=s,.glyphs=web_font_glyph_count(font)};
                if (!pass && u16(&p,s)==2) {
                    size_t def=s+u16(&p,s+10);
                    /* Match SDK/native table expansion: explicitly named
                     * second glyphs, including their explicit class zero. */
                    ctx.seconds=malloc((size_t)ctx.glyphs*2*sizeof(uint16_t));
                    if (!ctx.seconds) { p.bad=1; break; }
                    unsigned fmt=u16(&p,def),nclass=fmt==1 ? u16(&p,def+4) : u16(&p,def+2);
                    if (fmt==1 && span(&p,def+6,(size_t)nclass*2)) {
                        unsigned first=u16(&p,def+2);
                        for (unsigned j=0;j<nclass && !p.bad;j++) {
                            if (ctx.nsecond>=ctx.glyphs || first+j>=ctx.glyphs) { p.bad=1; break; }
                            ctx.seconds[ctx.nsecond*2]=(uint16_t)(first+j);
                            ctx.seconds[ctx.nsecond++*2+1]=(uint16_t)u16(&p,def+6+j*2);
                        }
                    } else if (fmt==2 && span(&p,def+4,(size_t)nclass*6)) {
                        for (unsigned j=0;j<nclass && !p.bad;j++) {
                            size_t r=def+4+j*6; unsigned first=u16(&p,r),last=u16(&p,r+2),cl=u16(&p,r+4);
                            if (last<first) { p.bad=1; break; }
                            for (unsigned g=first;g<=last && !p.bad;g++) {
                                if (ctx.nsecond>=ctx.glyphs || g>=ctx.glyphs) { p.bad=1; break; }
                                ctx.seconds[ctx.nsecond*2]=(uint16_t)g; ctx.seconds[ctx.nsecond++*2+1]=(uint16_t)cl;
                            }
                        }
                    } else p.bad=1;
                }
                if (!p.bad) coverage(&p,s+u16(&p,s+2),pass ? single : pair,&ctx);
                free(ctx.seconds);
            }
        }
    }
done:
    free(p.data);
    if (p.bad) { free(p.pairs); return 1; }
    if (p.count) qsort(p.pairs,p.count,sizeof *p.pairs,compare);
    *pairs=p.pairs; *count=p.count; return 0;
}
