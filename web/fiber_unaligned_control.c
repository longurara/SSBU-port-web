/* Negative-control allocator, linked only by the isolated regression fixture.
 * Simulates the 8-byte stack alignment permitted by Wasm32 malloc. The test
 * must abort at the EM_ASM varargs alignment assertion before finalization. */
#include <stddef.h>
void *__real_aligned_alloc(size_t,size_t);
void *__wrap_aligned_alloc(size_t alignment,size_t size){
    if(alignment==16&&size==1048576){
        char *allocation=__real_aligned_alloc(alignment,size+16);
        return allocation?allocation+8:NULL;
    }
    return __real_aligned_alloc(alignment,size);
}
