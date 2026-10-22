#ifndef WEB_HOST_GPU_SAMPLER_H
#define WEB_HOST_GPU_SAMPLER_H
/* The native frontend's 48-byte sampler value encoding; builders have room
 * for a device pointer after it. Descriptor registration copies this value. */
typedef struct {
    uint8_t min,mag,ws,wt,wr,cmode,cfunc,reduction;
    float lodmin,lodmax,bias,aniso,border[4];
    uint8_t border_kind,pad[3];float lod_snap;
} WebGpuSamplerParams;
_Static_assert(sizeof(WebGpuSamplerParams)==48,"sampler guest ABI size");
int web_gpu_sampler_import(Ctx *,const char *);
int web_gpu_registered_sampler(uint64_t pool,unsigned index,WebGpuSamplerParams *output);
#endif
