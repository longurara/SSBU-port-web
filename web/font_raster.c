/* TrueType outlines and grayscale rasterization through the pinned SDK's
 * FreeType port. SDK GlyphMap layout/effects follow runtime/nnfontll.c.
 * This module does not map guest memory or load assets from the network. */
#include "font_raster.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct WebFont { FT_Library library; FT_Face face; uint8_t *bytes; };
static void p16(uint8_t *p, int v) { uint16_t n = (uint16_t)v; memcpy(p, &n, 2); }
static void p32(uint8_t *p, int32_t v) { memcpy(p, &v, 4); }

int web_font_open(const void *bytes, size_t size, unsigned face, WebFont **out) {
    if (!out) return 1;
    *out = NULL;
    if (!bytes || !size || size > 32u * 1024u * 1024u || face > 63) return 1;
    WebFont *f = calloc(1, sizeof *f);
    if (!f) return 1;
    f->bytes = malloc(size);
    if (!f->bytes) { free(f); return 1; }
    memcpy(f->bytes, bytes, size);
    int error = FT_Init_FreeType(&f->library);
    if (!error) error = FT_New_Memory_Face(f->library, f->bytes, (FT_Long)size, face, &f->face);
    if (!error && (!FT_IS_SCALABLE(f->face) || !f->face->units_per_EM)) error = 1;
    /* Same Unicode cmap preference as the SDK: Microsoft UCS-4, then BMP. */
    if (!error) {
        FT_CharMap chosen = NULL;
        for (int i = 0; i < f->face->num_charmaps; ++i) {
            FT_CharMap c = f->face->charmaps[i];
            if (c->platform_id == 3 && c->encoding_id == 10) { chosen = c; break; }
            if (c->platform_id == 3 && c->encoding_id == 1) chosen = c;
        }
        error = chosen ? FT_Set_Charmap(f->face, chosen) : FT_Select_Charmap(f->face, FT_ENCODING_UNICODE);
    }
    if (error) { web_font_close(f); return error; }
    *out = f;
    return 0;
}
void web_font_close(WebFont *f) {
    if (!f) return;
    if (f->face) FT_Done_Face(f->face);
    if (f->library) FT_Done_FreeType(f->library);
    free(f->bytes); free(f);
}
unsigned web_font_glyph(WebFont *f, uint32_t code) { return f ? FT_Get_Char_Index(f->face, code) : 0; }
unsigned web_font_units(WebFont *f) { return f ? f->face->units_per_EM : 0; }
unsigned web_font_glyph_count(WebFont *f) { return f ? (unsigned)f->face->num_glyphs : 0; }
int web_font_table(WebFont *f, const char tag[4], void *out, size_t *size) {
    if (!f || !tag || !size || *size > LONG_MAX || (out && !*size)) return 1;
    FT_ULong n = (FT_ULong)*size;
    FT_ULong key = FT_MAKE_TAG(tag[0], tag[1], tag[2], tag[3]);
    int error = FT_Load_Sfnt_Table(f->face, key, 0, out, &n);
    *size = n;
    return error;
}
static unsigned b16(const uint8_t *p) { return (unsigned)p[0] << 8 | p[1]; }
static void font_name(WebFont *f, unsigned id, uint8_t *out, size_t capacity) {
    FT_SfntName chosen = {0}; int best = -1;
    for (FT_UInt i = 0; i < FT_Get_Sfnt_Name_Count(f->face); i++) {
        FT_SfntName name;
        if (FT_Get_Sfnt_Name(f->face,i,&name) || name.name_id != id) continue;
        int score = name.platform_id == 3 ? (name.language_id == 0x409 ? 3 : 2) :
                    name.platform_id == 0 ? 1 : -1;
        if (score > best) { best = score; chosen = name; }
    }
    if (best < 0 || !capacity) return;
    size_t n = chosen.string_len / 2;
    if (n >= capacity) n = capacity - 1;
    /* Same ASCII-facing metrics ABI as the native adapter. */
    for (size_t i = 0; i < n; i++) {
        unsigned c = b16(chosen.string + 2*i); out[i] = c >= 32 && c < 127 ? (uint8_t)c : '?';
    }
    out[n] = 0;
}
int web_font_metrics(WebFont *f, uint8_t out[0x93c]) {
    if (!f || !out) return 1;
    memset(out,0,0x93c);
    uint8_t head[54], hhea[36], os2[78], cmap[4 + 20*8];
    size_t n = sizeof head;
    if (web_font_table(f,"head",head,&n) || n != sizeof head) return 1;
    n = sizeof hhea;
    if (web_font_table(f,"hhea",hhea,&n) || n != sizeof hhea) return 1;
    p16(out,f->face->units_per_EM); p16(out+2,b16(head+44));
    n = sizeof os2;
    if (!web_font_table(f,"OS/2",os2,&n) && n == sizeof os2) {
        const unsigned src[] = {74,76,68,70,62,72};
        for (unsigned i = 0; i < 6; i++) p16(out+4+i*2,b16(os2+src[i]));
        p16(out+0x16,b16(os2+8));
    }
    for (unsigned i = 0; i < 3; i++) p16(out+0x10+i*2,b16(hhea+4+i*2));
    for (unsigned i = 0; i < 4; i++) p16(out+0x18+i*2,b16(head+36+i*2));
    n = 0; int cff = !web_font_table(f,"CFF ",NULL,&n);
    p16(out+0x20,cff ? 0x80 : 1);
    n = 0;
    if (!web_font_table(f,"cmap",NULL,&n) && n >= 4) {
        size_t available = n < sizeof cmap ? n : sizeof cmap;
        if (!web_font_table(f,"cmap",cmap,&available)) {
            unsigned count = b16(cmap+2);
            if (count > 20) count = 20;
            if (count > (available-4)/8) count = (unsigned)(available-4)/8;
            p16(out+0x22,count);
            for (unsigned i = 0; i < count; i++) {
                p16(out+0x24+i*8,b16(cmap+4+i*8)); p16(out+0x26+i*8,b16(cmap+6+i*8));
            }
        }
    }
    p32(out+0xc4,(int32_t)f->face->num_faces); p32(out+0xc8,(int32_t)f->face->face_index);
    font_name(f,1,out+0xcc,50); font_name(f,4,out+0xfe,50); font_name(f,0,out+0x130,0x800);
    p16(out+0x930,f->face->units_per_EM); p32(out+0x934,(int32_t)f->face->num_glyphs);
    return 0;
}
static int load(WebFont *f, uint32_t code) {
    return FT_Load_Glyph(f->face, web_font_glyph(f, code),
                         FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_TRANSFORM);
}
static int style_valid(const WebFontStyle *s) {
    if (!s || s->effect > 2 || s->outline > 10) return 0;
    for (unsigned i = 0; i < 4; i++)
        if (s->scale[i] < -(4096 << 16) || s->scale[i] > (4096 << 16)) return 0;
    return 1;
}
static int32_t scaled(FT_Pos units, int32_t scale, unsigned upem) {
    return (int32_t)(((int64_t)units * scale + upem / 2) / upem);
}
static void advance(WebFont *f, const WebFontStyle *s, int32_t *dx, int32_t *dy) {
    FT_Pos units = f->face->glyph->metrics.horiAdvance;
    *dx = scaled(units, s->scale[0], f->face->units_per_EM);
    *dy = scaled(units, s->scale[2], f->face->units_per_EM);
}
int web_font_kerning(WebFont *f, uint32_t first, uint32_t second,
                     const WebFontStyle *s, int32_t *dx, int32_t *dy) {
    if (!f || !dx || !dy || !style_valid(s)) return 1;
    FT_Vector v = {0};
    int error = FT_Get_Kerning(f->face,web_font_glyph(f,first),web_font_glyph(f,second),
                               FT_KERNING_UNSCALED,&v);
    if (!error) {
        *dx = (int32_t)((int64_t)v.x * s->scale[0] / f->face->units_per_EM);
        *dy = (int32_t)((int64_t)v.x * s->scale[2] / f->face->units_per_EM);
    }
    return error;
}
int web_font_advance(WebFont *f, uint32_t code, const WebFontStyle *s, int32_t *dx, int32_t *dy) {
    if (!f || !dx || !dy || !style_valid(s)) return 1;
    int error = load(f, code);
    if (!error) advance(f, s, dx, dy);
    return error;
}

/* SDK dilation alternates 4/8 neighbors and subtracts original coverage for
 * the unfilled effect. Padding is n+1, including one untouched border pixel. */
static int effect(uint8_t **bits, int *w, int *h, int *left, int *top, unsigned n, unsigned mode) {
    if (!mode || !n || !*w || !*h) return 0;
    int pad = (int)n + 1, width = *w + 2 * pad, height = *h + 2 * pad;
    size_t size = (size_t)width * height;
    uint8_t *buf = calloc(1, size), *tmp = malloc(size);
    if (!buf || !tmp) { free(buf); free(tmp); return 1; }
    for (int y = 0; y < *h; y++) memcpy(buf + (y + pad) * width + pad, *bits + y * *w, *w);
    for (unsigned pass = n; pass; pass--) {
        memcpy(tmp, buf, size);
        for (int y = 1; y < height - 1; y++) for (int x = 1; x < width - 1; x++) {
            int i = y * width + x;
            if (tmp[i]) { buf[i] = 255; continue; }
            uint8_t m = 0;
            for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
                if ((pass & 1) && dx && dy) continue;
                uint8_t v = tmp[i + dy * width + dx]; if (v > m) m = v;
            }
            buf[i] = m;
        }
    }
    if (mode == 1) for (int y = 0; y < *h; y++) for (int x = 0; x < *w; x++)
        buf[(y + pad) * width + x + pad] -= (*bits)[y * *w + x];
    free(tmp); free(*bits); *bits = buf; *w = width; *h = height; *left -= pad; *top += pad;
    return 0;
}
int web_font_raster(WebFont *f, uint32_t code, unsigned type, const WebFontStyle *s,
                    uint8_t **map, size_t *size) {
    if (!map || !size) return 1;
    *map = NULL; *size = 0;
    if (!f || type != 8 || !style_valid(s)) return 1;
    int error = load(f, code);
    if (error) return error;
    int32_t dx, dy; advance(f, s, &dx, &dy);
    FT_Outline *o = &f->face->glyph->outline;
    int w = 0, h = 0, left = 0, top = 0;
    uint8_t *bits = NULL;
    if (o->n_points) {
        /* NO_SCALE coordinates are font units. Convert directly to 26.6 pixels
         * before rasterizing; FT_Set_Transform is intentionally bypassed. */
        FT_Matrix m = { (FT_Fixed)((int64_t)s->scale[0] * 64 / f->face->units_per_EM),
                        (FT_Fixed)((int64_t)s->scale[1] * 64 / f->face->units_per_EM),
                        (FT_Fixed)((int64_t)s->scale[2] * 64 / f->face->units_per_EM),
                        (FT_Fixed)((int64_t)s->scale[3] * 64 / f->face->units_per_EM) };
        FT_Outline_Transform(o, &m);
        FT_BBox box; FT_Outline_Get_CBox(o, &box);
        left = (int)(box.xMin >> 6); top = (int)((box.yMax + 63) >> 6);
        int bottom = (int)(box.yMin >> 6);
        w = (int)((box.xMax + 63) >> 6) - left; h = top - bottom;
        if (w < 0 || h < 0 || w > 4096 || h > 4096) return 1;
        if (w && h) {
            bits = calloc((size_t)w, h);
            if (!bits) return 1;
            FT_Outline_Translate(o, -left * 64L, -bottom * 64L);
            FT_Bitmap bitmap = {0}; bitmap.rows = h; bitmap.width = w;
            bitmap.pitch = w; bitmap.buffer = bits; bitmap.num_grays = 256; bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
            error = FT_Outline_Get_Bitmap(f->library, o, &bitmap);
            if (error) { free(bits); return error; }
        } else { w = h = 0; }
    }
    if (effect(&bits, &w, &h, &left, &top, s->outline, s->effect)) { free(bits); return 1; }
    size_t n = 0x28 + (size_t)w * h;
    uint8_t *out = calloc(1, n);
    if (!out) { free(bits); return 1; }
    p32(out + 8, (int32_t)n); p16(out + 12, left); p16(out + 14, h ? top - 1 : 0);
    p16(out + 16, (dx + 0x8000) >> 16); p16(out + 18, (dy + 0x8000) >> 16);
    p32(out + 20, dx); p32(out + 24, dy);
    p16(out + 28, w); p16(out + 30, h); p16(out + 32, w); p16(out + 36, 8); p16(out + 38, type);
    if (bits) memcpy(out + 0x28, bits, (size_t)w * h);
    free(bits); *map = out; *size = n;
    return 0;
}
