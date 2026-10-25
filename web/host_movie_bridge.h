#ifndef WEB_HOST_MOVIE_BRIDGE_H
#define WEB_HOST_MOVIE_BRIDGE_H
#include <stdint.h>
#include <stddef.h>
enum { MOVIE_CREATE=1,MOVIE_APPEND,MOVIE_OPEN,MOVIE_TRACK,MOVIE_SELECT,MOVIE_PEEK,MOVIE_READ,MOVIE_ADVANCE,MOVIE_SEEK,MOVIE_CLOSE,
    MOVIE_CONFIGURE=20,MOVIE_SUBMIT,MOVIE_ACQUIRE,MOVIE_COPY,MOVIE_RELEASE,MOVIE_RESET,MOVIE_FLUSH,MOVIE_DECODER_CLOSE,MOVIE_EOS };
enum { MOVIE_OK=0,MOVIE_FAILED=0x1000,MOVIE_INVALID=0x100b,MOVIE_UNSUPPORTED=0x1006,MOVIE_END=0x2000,MOVIE_NO_INPUT=0x1011,MOVIE_NO_OUTPUT=0x100e };
typedef struct {int status;uint32_t value;} WebMovieReply;
/* TRACK writes 64 bytes: u32 codec/type/width/height/rate/channels/maxInput/fps,
 * i64 durationUs, u32 trackNumber/supported, reserved16.
 * PEEK writes24: u32 track/size, i64 timestampUs, u32 key/durationUs.
 * ACQUIRE writes40: u32 slot/bytes, i64 timestampUs, u32 more/flags,
 * u32 width/height/yStride/uvOffset. All fields are little endian.
 * APPEND and SUBMIT copy staging into JS; READ/COPY return actual media bytes.
 * Calls must originate in a pthread, as the completion runs on the JS worker.
 */
WebMovieReply web_movie_request(unsigned operation,uint32_t handle,int index,unsigned flags,int64_t time,void *staging,size_t length);
#endif
