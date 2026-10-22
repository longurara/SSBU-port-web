#ifndef WEB_RUNTIME_FRAME_H
#define WEB_RUNTIME_FRAME_H
#include <setjmp.h>
#include <stdint.h>
typedef struct WebRuntimeFrame {
    jmp_buf boundary;
    struct WebRuntimeFrame *previous;
    int import_symbol;
    uint64_t failure_address;
    char failure[160];
} WebRuntimeFrame;
/* Keep TLS lookup in a separate translation unit. A suspended C stack can
 * resume on another pthread; a cached address of that pthread's TLS cannot. */
void *web_runtime_frame_get(void);
void web_runtime_frame_set(void *frame);
/* Only a finalized, suspended fiber owns the chain passed here. */
void web_runtime_frame_discard(void *frame);
#endif
