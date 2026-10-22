/* Diagnostics for the pinned Emscripten dlmalloc build, never a guest allocator.
 * Its sysroot malloc.h omits mallinfo; this is the size_t ABI declared by the
 * bundled system/lib/dlmalloc.c (MALLINFO_FIELD_TYPE=size_t). */
#include <emscripten/emscripten.h>
#include <stddef.h>
#include <stdio.h>
struct mallinfo {
    size_t arena,ordblks,smblks,hblks,hblkhd,usmblks,fsmblks,uordblks,fordblks,keepcost;
};
extern struct mallinfo mallinfo(void);
EMSCRIPTEN_KEEPALIVE const char *web_allocation_info(void) {
    static char text[320];
    struct mallinfo info=mallinfo();
    snprintf(text,sizeof text,"{\"allocator\":\"dlmalloc\",\"allocatedBytes\":%zu,\"freeBytes\":%zu,\"systemBytes\":%zu,\"peakSystemBytes\":%zu,\"freeChunks\":%zu}",
        info.uordblks,info.fordblks,info.arena,info.usmblks,info.ordblks);
    return text;
}
