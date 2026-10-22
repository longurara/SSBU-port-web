#ifndef WEB_HOST_GPU_BRIDGE_H
#define WEB_HOST_GPU_BRIDGE_H
#include <stdint.h>
#include <stddef.h>
typedef struct { int status; uint32_t value; } WebGpuReply;
/* Operations: 1 allocate, 2 upload, 3 readback, 4 release, 5 flush, 6 finish,
 * 7 device error status, 8 fence snapshot, 9 CPU fence wait (uint64 ns staging),
 * 10 release fence, 11 server wait, 12 register shader sources,
 * 13 release program, 14 create/upload texture, 15 release texture,
 * 16 texture readback (offset=mip, size=layer), 17 submit staged texture copies.
 * 18 create texture view, 19 release texture view, 20 retain program snapshot,
 * 21 release program snapshot, 22 submit physical color clears (u32 count,
 * followed by 56-byte legacy records or 60-byte records with a view-format
 * override; see gpu-render.mjs), 23 present a retained window texture
 * (handle=physical texture, staging=int32 x,y,width,height crop).
 * 24 real graphics draw (bounded UTF-8 JSON frontend state, retained program
 * snapshots and resolved physical resources; see host_gpu_draw.c/gpu-draw.mjs).
 * 25 depth/stencil clears (u32 count followed by 48-byte subresource/value/
 * stencil-mask/scissor/format records; see gpu-render.mjs).
 * The NVN recorder captures
 * targets/scissors/colors and preserves copy/clear ordering. The calling pthread
 * waits while the owner Worker validates and completes the WebGPU request. */
WebGpuReply web_gpu_request(unsigned operation,uint32_t handle,uint64_t offset,uint64_t size,void *staging,size_t length);
uint64_t web_gpu_device_address(void);
uint32_t web_gpu_origin_mode(void);
int web_gpu_resources_import(Ctx *,const char *);
int web_gpu_resolve(uint64_t address,uint64_t size,uint32_t *handle,uint64_t *offset);
int web_gpu_draw_range(uint64_t address,uint64_t size,uint32_t *handle,uint64_t *offset);
int web_gpu_pool_hold(uint64_t pool,uint64_t offset,uint64_t size);
void web_gpu_pool_release(uint64_t pool);
int web_gpu_pool_copy(uint64_t pool,uint64_t offset,void *output,size_t size);
uint64_t web_gpu_pool_address(uint64_t pool,uint64_t offset);
int web_gpu_pool_hold_address(uint64_t address,uint64_t size,uint64_t *pool,uint64_t *offset);
/* sampler=0 selects a texture pool, sampler=1 a sampler pool. */
int web_gpu_descriptor_hold(uint64_t pool,int sampler);
void web_gpu_descriptor_release(uint64_t pool,int sampler);
int web_gpu_command_pool_binding(uint64_t handle,unsigned index,int *sampler,uint64_t *pool);
/* A nonzero snapshot owns immutable shader bytes until operation 21 releases it. */
int web_gpu_program_snapshot(uint64_t program,uint32_t *snapshot);
int web_gpu_command_program_binding(uint64_t handle,unsigned index,uint32_t *stages,uint32_t *snapshot);
uint32_t web_gpu_queue_program_snapshot(uint64_t queue,unsigned stage);
int web_gpu_commands_import(Ctx *,const char *);
int web_gpu_states_import(Ctx *,const char *);
int web_gpu_programs_import(Ctx *,const char *);
#endif
