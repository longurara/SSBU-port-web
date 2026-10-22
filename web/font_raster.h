#ifndef WEB_FONT_RASTER_H
#define WEB_FONT_RASTER_H
#include <stddef.h>
#include <stdint.h>

/* Private host objects. Callers serialize access to each font; no host pointer
 * may be exposed as a guest address. Input bytes are copied and owned here. */
typedef struct WebFont WebFont;
typedef struct {
    int32_t scale[4]; /* SDK matrix: xx, xy, yx, yy; 16.16 pixels per em */
    unsigned outline; /* 0..10 pixels */
    unsigned effect;  /* 0 normal, 1 unfilled outline, 2 filled outline */
} WebFontStyle;

int web_font_open(const void *bytes, size_t size, unsigned face, WebFont **out);
void web_font_close(WebFont *font);
unsigned web_font_glyph(WebFont *font, uint32_t code);
unsigned web_font_units(WebFont *font);
unsigned web_font_glyph_count(WebFont *font);
int web_font_table(WebFont *font, const char tag[4], void *out, size_t *size);
/* SDK Metrics, exactly 0x93c bytes, expressed in font units. */
int web_font_metrics(WebFont *font, uint8_t out[0x93c]);
int web_font_advance(WebFont *font, uint32_t code, const WebFontStyle *style,
                     int32_t *dx, int32_t *dy);
/* Returns an owned SDK GlyphMap (0x28-byte header + top-down gray coverage).
 * Release with free(). No shared scratch glyph: concurrent guest clients must
 * retain independent allocations until ReleasesGlyph. */
int web_font_raster(WebFont *font, uint32_t code, unsigned type,
                    const WebFontStyle *style, uint8_t **map, size_t *size);
#endif
