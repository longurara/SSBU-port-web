#ifndef WEB_HOST_GPU_DRAW_H
#define WEB_HOST_GPU_DRAW_H
#include "runtime.h"
#include "host_gpu_render.h"
typedef struct WebGpuDrawRecord WebGpuDrawRecord;
typedef struct WebGpuDrawState WebGpuDrawState;
int web_gpu_draw_known(const char *);
WebGpuDrawRecord *web_gpu_draw_record(Ctx *,const char *,const char **);
void web_gpu_draw_release(WebGpuDrawRecord *);
void web_gpu_draw_state_release(WebGpuDrawState *);
const char *web_gpu_draw_execute(WebGpuDrawState **,const WebGpuDrawRecord *,const WebGpuRenderState *,const uint32_t[6],uint64_t,uint64_t);
#endif
