#ifndef WEB_HOST_MOVIE_H
#define WEB_HOST_MOVIE_H
#include "runtime.h"
int web_movie_import(Ctx *c,const char *name);
/* Called only after all guest workers have stopped, alongside GuestMemory reset. */
void web_movie_reset(void);
unsigned web_movie_live_allocations(void);
unsigned web_movie_live_metadata(void);
#endif
