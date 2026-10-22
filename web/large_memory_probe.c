/* Dump-free test of shared wasm32 pointers above the signed 2GiB boundary. */
#include <emscripten/emscripten.h>
#include <stdint.h>
#include <stdlib.h>
static void *padding;
static uint8_t *high;
static uintptr_t last_token;
static int last_status, completions;
EMSCRIPTEN_KEEPALIVE uint32_t large_probe_begin(void) {
    padding=malloc(0x81000000u);
    if(!padding)return 0;
    high=malloc(4096);
    if(!high)return 0;
    for(unsigned i=0;i<64;i++)high[i]=(uint8_t)(i*37+11);
    return (uint32_t)(uintptr_t)high;
}
EMSCRIPTEN_KEEPALIVE void gpu_complete(uintptr_t token,int status,uint32_t value) {
    (void)value;last_token=token;last_status=status;completions++;
}
EMSCRIPTEN_KEEPALIVE int large_probe_verify(void) {
    if(!high||last_token!=(uintptr_t)high||last_status||completions!=2)return 0;
    for(unsigned i=0;i<64;i++)if(high[128+i]!=high[i])return 0;
    return 1;
}
EMSCRIPTEN_KEEPALIVE void large_probe_end(void) {free(high);free(padding);high=NULL;padding=NULL;}
