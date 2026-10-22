#ifndef WEB_HOST_GPU_TEXTURE_H
#define WEB_HOST_GPU_TEXTURE_H
#include "runtime.h"
/* Same portable builder encoding as runtime/nvn_front.h. The device lives in
 * the unused tail of the 128-byte guest object; every pointer stays virtual. */
typedef struct {
    int32_t flags,target,format,width,height,depth,levels,samples,dsmode,has_layout;
    uint8_t swz[4],layout[8];
    uint64_t pool,offset,packaged;
    int64_t stride;
    uint32_t glname;
} WebGpuTextureBuilder;
_Static_assert(sizeof(WebGpuTextureBuilder)==96,"native texture-builder encoding");
typedef struct {uint8_t set,target,dsmode,pad;uint16_t format;int16_t lvl0,nlvl,lay0,nlay;uint8_t swz[4];} WebGpuTextureView;
_Static_assert(sizeof(WebGpuTextureView)==18,"native texture-view encoding");
uint64_t web_nvn_layout_query(Ctx *,const WebGpuTextureBuilder *,unsigned function,uint32_t output[4]);
int web_gpu_texture_import(Ctx *,const char *);
int web_gpu_texture_object_import(Ctx *,const char *);
int web_gpu_texture_handle(uint64_t object,uint32_t *handle);
int web_gpu_texture_view_import(Ctx *,const char *);
typedef struct WebGpuTextureRef WebGpuTextureRef;
WebGpuTextureRef *web_gpu_texture_hold(uint64_t object);
void web_gpu_texture_release(WebGpuTextureRef *);
void web_gpu_texture_snapshot(const WebGpuTextureRef *,WebGpuTextureBuilder *,uint32_t *handle);
typedef struct {uint32_t texture,view_handle;int image;WebGpuTextureBuilder builder;WebGpuTextureView view;} WebGpuTextureBinding;
int web_gpu_registered_texture(uint64_t pool,unsigned index,WebGpuTextureBinding *output);
#endif
