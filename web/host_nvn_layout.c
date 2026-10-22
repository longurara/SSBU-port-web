/* Execute the source SDK's texture-storage rules. This device model describes
 * Switch archive layout; it never supplies physical WebGPU capability values. */
#include "host_gpu_texture.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
void web_nvn_layout_dispatch(Ctx *,uint64_t);
static pthread_mutex_t layout_init_lock=PTHREAD_MUTEX_INITIALIZER;
static atomic_uint layout_queries;
static WebGpuTextureBuilder last_builder;
static unsigned last_function;
static uint64_t last_result;
static char layout_info[512];
unsigned web_nvn_layout_query_count(void){return atomic_load(&layout_queries);}
const char *web_nvn_layout_info(void) {
    pthread_mutex_lock(&layout_init_lock);
    snprintf(layout_info,sizeof layout_info,"{\"queries\":%u,\"function\":\"%x\",\"result\":\"%llx\",\"format\":%u,\"target\":%d,\"flags\":%u,\"width\":%d,\"height\":%d,\"depth\":%d,\"levels\":%d,\"samples\":%d}",
        atomic_load(&layout_queries),last_function,(unsigned long long)last_result,(unsigned)last_builder.format,last_builder.target,
        (unsigned)last_builder.flags,last_builder.width,last_builder.height,last_builder.depth,last_builder.levels,last_builder.samples);
    pthread_mutex_unlock(&layout_init_lock);return layout_info;
}
static const uint64_t SDK=0x7500000000ULL,MODEL=0x7901000000ULL;
static int initialise(void) {
    GuestMemory *memory=web_memory_current();uint32_t marker=0;
    if(gm_read(memory,MODEL+0x1ff0,&marker,4)==GM_OK&&marker==0x4e564e4c)return 1;
    FILE *file=fopen("/sdk-layout-image.bin","rb");if(!file)return 0;
    int ok=gm_reserve(memory,SDK,0xcfb000)==GM_OK&&gm_commit(memory,SDK,0xcfb000)==GM_OK&&
        gm_reserve(memory,MODEL,0x4000)==GM_OK&&gm_commit(memory,MODEL,0x4000)==GM_OK;
    uint8_t bytes[8192];
    for(size_t offset=0;ok&&offset<0xcfb000;) {
        size_t size=0xcfb000-offset;if(size>sizeof bytes)size=sizeof bytes;
        ok=fread(bytes,1,size,file)==size&&gm_write(memory,SDK+offset,bytes,size)==GM_OK;offset+=size;
    }
    if(ok&&fgetc(file)!=EOF)ok=0;fclose(file);if(!ok)return 0;
    wr64(MODEL+0x2200,SDK+0xc114e8);wr64(MODEL+0x2208,SDK+0xc11e88);
    wr64(MODEL+0x2000,SDK+0xc11978);wr64(MODEL+0x2140,MODEL+0x2200);
    wr32(MODEL+0x90,0x10000);wr64(MODEL+0x928,MODEL+0x2000);
    web_read(SDK+0x9f19a0,bytes,16);web_write(MODEL+0x19a8,bytes,16);
    wr32(MODEL+0x1ac8,0);wr8(MODEL+0x1acc,(rd32(MODEL+0x19b0)&6)==4?0:1);
    if(gm_protect(memory,SDK,0xcfb000,0)!=GM_OK)return 0;
    wr32(MODEL+0x1ff0,0x4e564e4c);return 1;
}
uint64_t web_nvn_layout_query(Ctx *c,const WebGpuTextureBuilder *b,unsigned function,uint32_t output[4]) {
    if(b->glname)guest_trap("OpenGL texture interop is unavailable on WebGPU",b->glname);
    pthread_mutex_lock(&layout_init_lock);int ready=initialise();pthread_mutex_unlock(&layout_init_lock);
    if(!ready)guest_trap("cannot map SDK texture layout image/model",SDK);
    Ctx guest=*c;uint64_t scratch=(guest.x[31]-0x100)&~15ULL;uint8_t builder[128]={0};
    uint64_t device=MODEL;memcpy(builder+8,&device,8);
    const uint32_t fields[]={b->flags,b->target,b->width,b->height,b->depth,b->levels,b->format,b->samples};
    memcpy(builder+0x20,fields,sizeof fields);
    if(b->has_layout){uint16_t layout=(uint16_t)(1|((b->layout[0]|(b->layout[1]<<8))&0xfff)<<4);memcpy(builder+0x60,&layout,2);}
    uint32_t stride=(uint32_t)b->stride;memcpy(builder+0x64,&stride,4);
    web_write(scratch,builder,sizeof builder);
    guest.x[0]=scratch;guest.x[1]=scratch+0x80;guest.x[31]=scratch;guest.x[30]=0x7300000000ULL;
    web_nvn_layout_dispatch(&guest,SDK+function);
    pthread_mutex_lock(&layout_init_lock);
    last_builder=*b;last_function=function;last_result=guest.x[0];atomic_fetch_add(&layout_queries,1);
    pthread_mutex_unlock(&layout_init_lock);
    if(output)web_read(scratch+0x80,output,16);
    return guest.x[0];
}
