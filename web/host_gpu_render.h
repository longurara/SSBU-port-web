#ifndef WEB_HOST_GPU_RENDER_H
#define WEB_HOST_GPU_RENDER_H
#include "runtime.h"
typedef struct WebGpuRenderRecord WebGpuRenderRecord;
typedef struct WebGpuRenderState WebGpuRenderState;
typedef struct {uint32_t handle,mip,layer,format,layers;} WebGpuDrawTarget;
const char *web_gpu_render_draw_targets(const WebGpuRenderState *,WebGpuDrawTarget[9],int32_t[4],uint32_t[2]);
int web_gpu_render_known(const char *);
WebGpuRenderRecord *web_gpu_render_record(Ctx *,const char *,const char **);
void web_gpu_render_release(WebGpuRenderRecord *);
const char *web_gpu_render_execute(WebGpuRenderState **,const WebGpuRenderRecord *);
void web_gpu_render_state_release(WebGpuRenderState *);
#endif
