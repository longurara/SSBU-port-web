#ifndef WEB_HOST_FONT_H
#define WEB_HOST_FONT_H
#include "runtime.h"
int web_font_import(Ctx *context, const char *name);
/* Only after all runtime workers stop. Does not dereference old guest memory. */
void web_font_reset(void);
#endif
