/* Local actual-font fixture; argv paths are never packaged in the module. */
#include "font_raster.h"
#include "font_otf.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int16_t r16(const uint8_t *p) { int16_t v; memcpy(&v,p,2); return v; }
static int32_t r32(const uint8_t *p) { int32_t v; memcpy(&v,p,4); return v; }
static unsigned coverage(const uint8_t *p, size_t n) {
    unsigned sum = 0; for (size_t i = 40; i < n; i++) sum += p[i]; return sum;
}
int main(int argc, char **argv) {
    assert(argc > 1);
    WebFont *bad = NULL;
    assert(web_font_open("not a font",10,0,&bad) && !bad);
    unsigned tested = 0;
    for (int arg = 1; arg < argc; arg++) {
        FILE *file = fopen(argv[arg],"rb"); assert(file);
        assert(!fseek(file,0,SEEK_END)); long length = ftell(file); assert(length > 0);
        rewind(file); uint8_t *bytes = malloc(length); assert(bytes);
        assert(fread(bytes,1,length,file) == (size_t)length); fclose(file);
        WebFont *f = NULL; assert(!web_font_open(bytes,length,0,&f));
        /* Ownership is independent of the input buffer. */
        memset(bytes,0,length); free(bytes);
        assert(web_font_units(f) == (strstr(argv[arg],"zh-tw") ? 1000 : 1024));
        assert(web_font_glyph_count(f) > 800);
        assert(!web_font_glyph(f,0x10ffff));
        uint8_t head[54]; size_t headsize = sizeof head;
        assert(!web_font_table(f,"head",head,&headsize) && headsize == sizeof head);
        assert(((unsigned)head[18] << 8 | head[19]) == web_font_units(f));
        uint8_t metrics[0x93c]; assert(!web_font_metrics(f,metrics));
        assert((unsigned)r16(metrics) == web_font_units(f));
        assert(r16(metrics+0x930) == r16(metrics));
        assert((unsigned)r32(metrics+0x934) == web_font_glyph_count(f));
        assert(r16(metrics+4) > 0 && r16(metrics+6) >= 0);
        assert(metrics[0xcc] && metrics[0xfe] && r16(metrics+0x22) > 0);
        WebFontPair *pairs=NULL; size_t pair_count=0;
        assert(!web_font_otf(f,0,&pairs,&pair_count));
        for (size_t i=1;i<pair_count;i++) assert(pairs[i-1].first < pairs[i].first ||
            (pairs[i-1].first == pairs[i].first && pairs[i-1].second <= pairs[i].second));
        printf("OTF entries=%zu\n",pair_count); free(pairs);
        /* Extension fonts need not contain Latin; choose a real cmap entry. */
        uint32_t code = 'A';
        if (!web_font_glyph(f,code)) for (code = 33; code < 0xffff; code++)
            if (web_font_glyph(f,code)) break;
        assert(code < 0xffff && web_font_glyph(f,code));
        WebFontStyle s = {{32 << 16,0,0,32 << 16},0,0};
        const uint32_t international[] = {0x3042,0x4e2d,0xac00,0x25a0};
        unsigned international_count = 0;
        for (unsigned i = 0; i < sizeof international / sizeof *international; i++) {
            if (!web_font_glyph(f,international[i])) continue;
            uint8_t *sample = NULL; size_t sample_size = 0;
            assert(!web_font_raster(f,international[i],8,&s,&sample,&sample_size));
            assert(r16(sample+28) > 0 && r16(sample+30) > 0 && coverage(sample,sample_size));
            free(sample); international_count++;
        }
        /* The small simplified-Chinese extension may contain none of these. */
        assert(international_count || strstr(argv[arg],"ext_zh-cn"));
        uint8_t *a = NULL; size_t na = 0;
        assert(!web_font_raster(f,code,8,&s,&a,&na));
        assert(r16(a+28) > 0 && r16(a+30) > 0 && coverage(a,na));
        assert(r32(a+8) == (int32_t)na && r16(a+32) == r16(a+28));
        int32_t dx,dy; assert(!web_font_advance(f,code,&s,&dx,&dy));
        assert(r32(a+20) == dx && r32(a+24) == dy && !dy);
        /* A second acquisition cannot overwrite a retained glyph. */
        uint8_t *saved = malloc(na); assert(saved); memcpy(saved,a,na);
        s.scale[0] *= 2; s.scale[3] *= 2;
        uint8_t *b = NULL; size_t nb = 0;
        assert(!web_font_raster(f,code,8,&s,&b,&nb));
        assert(r16(b+28) >= r16(a+28) && r16(b+30) >= r16(a+30));
        assert(coverage(b,nb) > coverage(a,na) && !memcmp(a,saved,na));
        free(b); s.scale[0] /= 2; s.scale[3] /= 2;
        s.outline = 2; s.effect = 2;
        assert(!web_font_raster(f,code,8,&s,&b,&nb));
        assert(r16(b+28) == r16(a+28)+6 && r16(b+30) == r16(a+30)+6);
        assert(r16(b+12) == r16(a+12)-3 && r16(b+14) == r16(a+14)+3);
        unsigned filled = coverage(b,nb); free(b); s.effect = 1;
        assert(!web_font_raster(f,code,8,&s,&b,&nb));
        assert(coverage(b,nb) + coverage(a,na) == filled);
        free(b); s.effect = s.outline = 0;
        s.scale[0] = s.scale[3] = 0; s.scale[1] = -(32 << 16); s.scale[2] = 32 << 16;
        assert(!web_font_raster(f,code,8,&s,&b,&nb));
        assert(r16(b+28) == r16(a+30) && r16(b+30) == r16(a+28));
        assert(r32(b+20) == 0 && r32(b+24) == dx); free(b);
        s.scale[0] = s.scale[3] = 32 << 16; s.scale[1] = s.scale[2] = 0;
        if (web_font_glyph(f,' ')) {
            assert(!web_font_raster(f,' ',8,&s,&b,&nb));
            assert(nb == 40 && !r16(b+28) && !r16(b+30) && r32(b+20) > 0); free(b);
        }
        /* Exercise the off-diagonal matrix and negative scale. */
        s.scale[1] = 8 << 16; s.scale[2] = 4 << 16;
        assert(!web_font_raster(f,code,8,&s,&b,&nb)); assert(coverage(b,nb)); free(b);
        s.scale[0] = -(32 << 16); s.scale[1] = s.scale[2] = 0;
        assert(!web_font_raster(f,code,8,&s,&b,&nb)); assert(coverage(b,nb)); free(b);
        assert(web_font_raster(f,code,1,&s,&b,&nb) && !b && !nb);
        printf("font %u units=%u glyphs=%u code=U+%04X bitmap=%dx%d coverage=%u CJK/symbols=%u\n",
               ++tested,web_font_units(f),web_font_glyph_count(f),code,r16(a+28),r16(a+30),coverage(a,na),international_count);
        free(a); free(saved); web_font_close(f);
    }
    printf("PASS: %u real local fonts; raster, metrics, effects, transforms, ownership\n",tested);
    return 0;
}
