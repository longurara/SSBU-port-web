#ifndef WEB_HOST_MOVIE_H
#define WEB_HOST_MOVIE_H
#include "runtime.h"
int web_movie_import(Ctx *c,const char *name);
/* Called only after all guest workers have stopped, alongside GuestMemory reset. */
void web_movie_reset(void);
unsigned web_movie_live_allocations(void);
unsigned web_movie_live_metadata(void);
uint64_t web_movie_guest_call(Ctx *,uint64_t function,uint64_t a0,uint64_t a1,uint64_t a2,uint64_t a3,uint64_t a4);
uint64_t web_movie_guest_alloc(Ctx *,uint64_t size);
void web_movie_guest_free(Ctx *,uint64_t address);
enum {WEB_MOVIE_I32=1,WEB_MOVIE_I64,WEB_MOVIE_FLOAT,WEB_MOVIE_POINTER,WEB_MOVIE_STRING};
int web_movie_metadata_set(Ctx *,uint64_t object,const char *key,unsigned type,uint64_t value);
int web_movie_metadata_string(Ctx *,uint64_t object,const char *key,const char *value);
int web_movie_metadata_get(uint64_t object,const char *key,unsigned type,uint64_t *value);
void web_movie_metadata_clear(Ctx *,uint64_t object);
int web_movie_codec_import(Ctx *,const char *name);
int web_movie_codec_indirect(Ctx *,uint64_t address);
void web_movie_codec_reset(void);
unsigned web_movie_live_extractors(void);
int web_movie_decoder_import(Ctx *,const char *name);
int web_movie_decoder_indirect(Ctx *,uint64_t address);
void web_movie_decoder_reset(void);
unsigned web_movie_live_decoders(void);
void web_movie_notify(uint32_t owner,unsigned mask);
#endif
