#ifndef WEB_FONT_OTF_H
#define WEB_FONT_OTF_H
#include "font_raster.h"
typedef struct { uint16_t first, second; int16_t advance, placement; } WebFontPair;
/* Sorted SDK entries. Caller owns *pairs (free); missing GPOS succeeds empty.
 * Parses kern pair adjustments and optional palt single adjustments. */
int web_font_otf(WebFont *font, int kern_only, WebFontPair **pairs, size_t *count);
#endif
