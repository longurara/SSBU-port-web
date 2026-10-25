#ifndef MOVIE_SDK_PROBE_SUPPORT_H
#define MOVIE_SDK_PROBE_SUPPORT_H
#include "host_movie_bridge.h"
int movie_sdk_probe_begin(void);
WebMovieReply movie_sdk_probe_open(uint32_t *backend);
WebMovieReply movie_sdk_probe_track(int index,void *info);
WebMovieReply movie_sdk_probe_select(int index);
WebMovieReply movie_sdk_probe_peek(void *sample);
WebMovieReply movie_sdk_probe_read(void *bytes,size_t size);
WebMovieReply movie_sdk_probe_advance(void);
WebMovieReply movie_sdk_probe_close(void);
int movie_sdk_probe_finish(void);
unsigned movie_sdk_probe_reader_calls(void);
uint64_t movie_sdk_probe_extractor(void);
uint64_t movie_sdk_probe_metadata(void);
#endif
