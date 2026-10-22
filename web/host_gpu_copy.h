#ifndef WEB_HOST_GPU_COPY_H
#define WEB_HOST_GPU_COPY_H
#include "runtime.h"
typedef struct WebGpuCopy WebGpuCopy;
typedef struct WebGpuCopyLease WebGpuCopyLease;
WebGpuCopy *web_gpu_copy_record(Ctx *,uint64_t row_stride,uint64_t image_stride,const char **error);
void web_gpu_copy_release(WebGpuCopy *);
size_t web_gpu_copy_packet_size(const WebGpuCopy *);
WebGpuCopyLease *web_gpu_copy_prepare(const WebGpuCopy *,uint8_t *output);
void web_gpu_copy_release_lease(WebGpuCopyLease *);
#endif
