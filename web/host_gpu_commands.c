/* Command-buffer allocation and recording lifecycle. Unsupported recording
 * operations stop before they can produce a submitted/no-op game command. */
#include "runtime.h"
#include "host_gpu_bridge.h"
#include "host_gpu_copy.h"
#include "host_gpu_render.h"
#include "host_gpu_draw.h"
#include "host_gpu_texture.h"
#include <emscripten/emscripten.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct CommandList;
int web_runtime_thread_call(Ctx *,uint64_t);
typedef struct ProgramSnapshot {uint32_t token,references;} ProgramSnapshot;
typedef struct PoolBinding {uint64_t pool;int sampler;WebGpuCopy *copy;WebGpuRenderRecord *render;WebGpuDrawRecord *draw;int program_binding;uint32_t stages;ProgramSnapshot *program;struct CommandList *sublist;struct PoolBinding *next;} PoolBinding;
typedef struct CommandBuffer {
    uint64_t key,pool,command_size,command_used,control,control_size,control_used,callback,callback_data;
    int recording,callback_enabled,in_callback;
    uint64_t row_stride,image_stride,record_bytes;
    PoolBinding *bindings,*bindings_tail;
    char label[128];
    struct CommandBuffer *next;
} CommandBuffer;
typedef struct CommandList {uint64_t handle,command_bytes;unsigned references;PoolBinding *bindings;struct CommandList *next;} CommandList;
typedef struct Queue {uint64_t key,command_size,compute_size,control_size,command_total,control_total,texture_pool,sampler_pool;
    uint32_t flags,priority,references;ProgramSnapshot *programs[6];WebGpuRenderState *render;WebGpuDrawState *draw;int alive;pthread_mutex_t submit_lock;char label[128];struct Queue *next;} Queue;
typedef struct Fence {uint32_t handle,references;} Fence;
typedef struct Sync {uint64_t key;Fence *fence;int signaled;char label[128];struct Sync *next;} Sync;
typedef struct Window {uint64_t key,native,textures[6];int count,interval,nactive,crop[4];uint32_t next_index;char label[128];struct Window *next;} Window;
static CommandBuffer *buffers[1024];
static CommandList *lists;
static Queue *queues;
static Sync *syncs;
static Window *windows;
static uint32_t next_list=1;
static pthread_mutex_t commands_lock=PTHREAD_MUTEX_INITIALIZER;
EMSCRIPTEN_KEEPALIVE const char *web_gpu_command_info(void) {
    static char text[384];
    // A guest may wait for a GPU reply while holding this lock. The owner
    // Worker must skip the observation rather than block its reply loop.
    if(pthread_mutex_trylock(&commands_lock)){snprintf(text,sizeof text,"{\"busy\":true}");return text;}
    size_t count=0,bindings=0,draws=0,renders=0,programs=0,sublists=0;uint64_t bytes=0;
    for(CommandList *list=lists;list;list=list->next){count++;bytes+=list->command_bytes;
        for(PoolBinding *b=list->bindings;b;b=b->next){bindings++;draws+=b->draw!=NULL;renders+=b->render!=NULL;programs+=b->program_binding!=0;sublists+=b->sublist!=NULL;}}
    pthread_mutex_unlock(&commands_lock);
    snprintf(text,sizeof text,"{\"registryLists\":%zu,\"registryBindings\":%zu,\"drawRecords\":%zu,\"renderRecords\":%zu,\"programBindings\":%zu,\"sublistBindings\":%zu,\"modeledCommandBytes\":%llu}",count,bindings,draws,renders,programs,sublists,(unsigned long long)bytes);
    return text;
}
static unsigned bucket(uint64_t key){return (unsigned)(key>>4)&1023;}
static void fail(const char *what,uint64_t address){pthread_mutex_unlock(&commands_lock);guest_trap(what,address);}
static CommandBuffer *find(uint64_t key) {for(CommandBuffer *b=buffers[bucket(key)];b;b=b->next)if(b->key==key)return b;return NULL;}
static void require_memory(Ctx *c,CommandBuffer *buffer,uint64_t needed,int control) {
    uint64_t size=control?buffer->control_size:buffer->command_size,used=control?buffer->control_used:buffer->command_used;
    if(used<=size&&needed<=size-used)return;
    if(!buffer->callback||(!control&&!buffer->callback_enabled))fail(control?"NVN control memory exhausted":"NVN command memory exhausted",buffer->key);
    if(buffer->in_callback)fail("recursive NVN memory refill",buffer->key);
    if(needed>UINT64_MAX-255)fail("NVN memory refill size overflow",buffer->key);
    const uint64_t key=buffer->key,callback=buffer->callback,user=buffer->callback_data;
    Ctx saved=*c;buffer->in_callback=1;
    c->x[0]=key;c->x[1]=control?1:0;c->x[2]=(needed+255)&~255ULL;c->x[3]=user;
    pthread_mutex_unlock(&commands_lock);
    int ok=web_runtime_thread_call(c,callback);
    pthread_mutex_lock(&commands_lock);
    if(find(key)!=buffer)fail("NVN command buffer changed during refill",key);
    buffer->in_callback=0;
    // Retain architectural exclusive-monitor effects, while restoring the
    // outer NVN call's register arguments and diagnostic context.
    uint64_t ex_addr=c->ex_addr,ex_val=c->ex_val,ex_fail=c->ex_fail;*c=saved;
    c->ex_addr=ex_addr;c->ex_val=ex_val;c->ex_fail=ex_fail;
    if(!ok)fail("NVN memory callback failed",key);
    size=control?buffer->control_size:buffer->command_size;used=control?buffer->control_used:buffer->command_used;
    if(used>size||needed>size-used)fail(control?"NVN callback supplied insufficient control memory":"NVN callback supplied insufficient command memory",key);
}
static void release_program(ProgramSnapshot *program) {
    if(program&&!--program->references){
        if(web_gpu_request(21,program->token,0,0,NULL,0).status)fail("GPU program snapshot release failed",program->token);
        free(program);
    }
}
static void release_list(CommandList *list);
static void free_bindings(PoolBinding *binding) {
    while(binding){PoolBinding *next=binding->next;if(binding->sublist)release_list(binding->sublist);else if(binding->render)web_gpu_render_release(binding->render);else if(binding->draw)web_gpu_draw_release(binding->draw);else if(binding->program_binding)release_program(binding->program);else if(binding->copy)web_gpu_copy_release(binding->copy);else web_gpu_descriptor_release(binding->pool,binding->sampler);free(binding);binding=next;}
}
static void release_list(CommandList *list){if(!--list->references){free_bindings(list->bindings);free(list);}}
static void release_queue(Queue *queue) {
    if(!--queue->references) {
        if(queue->texture_pool)web_gpu_descriptor_release(queue->texture_pool,0);
        if(queue->sampler_pool)web_gpu_descriptor_release(queue->sampler_pool,1);
        for(unsigned stage=0;stage<6;stage++)release_program(queue->programs[stage]);
        web_gpu_render_state_release(queue->render);
        web_gpu_draw_state_release(queue->draw);
        pthread_mutex_destroy(&queue->submit_lock);free(queue);
    }
}
int web_gpu_command_pool_binding(uint64_t handle,unsigned index,int *sampler,uint64_t *pool) {
    pthread_mutex_lock(&commands_lock);CommandList *list=lists;
    while(list&&list->handle!=handle)list=list->next;
    PoolBinding *binding=list?list->bindings:NULL;while(binding&&index--)binding=binding->next;
    int found=binding&&!binding->copy&&!binding->render&&!binding->draw&&!binding->sublist&&!binding->program_binding;if(found){*sampler=binding->sampler;*pool=binding->pool;}
    pthread_mutex_unlock(&commands_lock);return found;
}
int web_gpu_command_program_binding(uint64_t handle,unsigned index,uint32_t *stages,uint32_t *snapshot) {
    pthread_mutex_lock(&commands_lock);CommandList *list=lists;
    while(list&&list->handle!=handle)list=list->next;
    PoolBinding *record=list?list->bindings:NULL;while(record&&index--)record=record->next;
    int found=record&&record->program_binding;
    if(found){*stages=record->stages;*snapshot=record->program?record->program->token:0;}
    pthread_mutex_unlock(&commands_lock);return found;
}
uint32_t web_gpu_queue_program_snapshot(uint64_t key,unsigned stage) {
    pthread_mutex_lock(&commands_lock);Queue *queue=queues;while(queue&&queue->key!=key)queue=queue->next;
    uint32_t token=queue&&stage<6&&queue->programs[stage]?queue->programs[stage]->token:0;
    pthread_mutex_unlock(&commands_lock);return token;
}
static void copy_label(char *target,uint64_t source) {
    unsigned i=0;if(source)for(;i<127;i++){target[i]=(char)rd8(source+i);if(!target[i])return;}target[i]=0;
}
static int known(const char *operation) {
    if(web_gpu_render_known(operation)||web_gpu_draw_known(operation))return 1;
    const char *names[]={"Initialize","Finalize","AddCommandMemory","AddControlMemory","GetCommandMemorySize","GetControlMemorySize",
      "GetCommandMemoryUsed","GetControlMemoryUsed","GetCommandMemoryFree","GetControlMemoryFree","IsRecording","BeginRecording","EndRecording",
      "SetMemoryCallback","SetMemoryCallbackData","GetMemoryCallback","GetMemoryCallbackData","SetCommandMemoryCallbackEnabled","SetDebugLabel",
      "SetTexturePool","SetSamplerPool","BindProgram","CopyBufferToTexture","SetCopyRowStride","GetCopyRowStride","SetCopyImageStride","GetCopyImageStride",
      "SetShaderScratchMemory","SetTiledCacheAction","SetTiledCacheTileSize","SaveZCullData","RestoreZCullData","SetStencilCullCriteria","ResolveDepthBuffer",
      "SetColorReductionEnable","SetColorReductionThresholds","Barrier","DiscardColor","DiscardDepthStencil",
      "PushDebugGroup","PushDebugGroupStatic","PushDebugGroupDynamic","PopDebugGroup","PopDebugGroupId",
      "InsertDebugMarker","InsertDebugMarkerStatic","InsertDebugMarkerDynamic",
      "BindBlendState","BindChannelMaskState","BindColorState","BindDepthStencilState","BindMultisampleState","BindPolygonState",
      "BindVertexAttribState","BindVertexStreamState","BindVertexBuffer","BindVertexBuffers",
      "BindUniformBuffer","BindUniformBuffers","BindStorageBuffer","BindStorageBuffers",
      "BindTexture","BindTextures","BindImage","BindImages",
      "BindSeparateTexture","BindSeparateTextures","BindSeparateSampler","BindSeparateSamplers",
      "BindTransformFeedbackBuffer","BindTransformFeedbackBuffers",
      "SetViewport","SetViewports","SetViewportSwizzles","SetDepthRange","SetDepthRanges","SetDepthBounds",
      "SetStencilValueMask","SetStencilMask","SetStencilRef","SetBlendColor",
      "SetPointSize","SetLineWidth","SetPolygonOffsetClamp","SetAlphaRef","SetSampleMask",
      "SetRasterizerDiscard","SetDepthClamp","SetConservativeRasterEnable","SetConservativeRasterDilate","SetSubpixelPrecisionBias",
      "SetPatchSize","SetInnerTessellationLevels","SetOuterTessellationLevels","SetPrimitiveRestart",
      "ClearDepthStencil","ClearDepthStencili","UpdateUniformBuffer","WaitEvent","SignalEvent",
      "CallCommands","CopyCommands",
      "ClearBuffer","ClearTexture","ClearTexturei","ClearTextureui",
      "CopyBufferToBuffer","CopyTextureToBuffer","CopyTextureToTexture",
      "Downsample","DownsampleTextureView","TiledDownsample","TiledDownsampleTextureView",
      "BeginTransformFeedback","EndTransformFeedback","PauseTransformFeedback","ResumeTransformFeedback",
      "BindCoverageModulationTable","SetProgramSubroutines","SetRenderEnable","SetRenderEnableConditional",
      "ReportCounter","ReportValue","ResetCounter","FenceSync","WaitSync"};
    for(unsigned i=0;i<sizeof names/sizeof *names;i++)if(!strcmp(operation,names[i]))return 1;
    return 0;
}
static uint64_t queue_memory_size(uint64_t builder) {
    uint64_t command=rd64(builder+16),compute=rd64(builder+24),control=rd64(builder+32);
    if(command>0x40000000||compute>0x40000000||control>0x40000000)return 0;
    return (command+compute+control+4095)&~4095ULL;
}
static Sync *find_sync(uint64_t key) {for(Sync *sync=syncs;sync;sync=sync->next)if(sync->key==key)return sync;return NULL;}
static Window *find_window(uint64_t key) {for(Window *win=windows;win;win=win->next)if(win->key==key)return win;return NULL;}
static void release_fence(Fence *fence) {
    if(fence&&!--fence->references) {
        if(web_gpu_request(10,fence->handle,0,0,NULL,0).status)fail("GPU fence release failed",fence->handle);
        free(fence);
    }
}
static int sync_import(Ctx *c,const char *name) {
    if(strncmp(name,"nvnSync",7))return 0;
    const char *operation=name+7;
    if(strcmp(operation,"Initialize")&&strcmp(operation,"Finalize")&&strcmp(operation,"Wait")&&strcmp(operation,"SetDebugLabel"))return 0;
    pthread_mutex_lock(&commands_lock);Sync **link=&syncs;while(*link&&(*link)->key!=c->x[0])link=&(*link)->next;
    Sync *sync=*link;
    if(!strcmp(operation,"Initialize")) {
        int valid=c->x[0]&&!sync&&c->x[1]==web_gpu_device_address();
        if(valid){sync=calloc(1,sizeof *sync);if(sync){sync->key=c->x[0];sync->next=syncs;syncs=sync;}}
        c->x[0]=valid&&sync!=NULL;
    } else if(!strcmp(operation,"Finalize")) {
        if(sync){*link=sync->next;release_fence(sync->fence);free(sync);}
    } else if(!strcmp(operation,"Wait")) {
        Fence *fence=sync?sync->fence:NULL;
        if(!fence){c->x[0]=(sync&&sync->signaled)?0:3;pthread_mutex_unlock(&commands_lock);return 1;}
        fence->references++;uint64_t timeout=c->x[1];
        pthread_mutex_unlock(&commands_lock);
        WebGpuReply reply=web_gpu_request(9,fence->handle,0,0,&timeout,sizeof timeout);
        pthread_mutex_lock(&commands_lock);release_fence(fence);c->x[0]=reply.status?3:reply.value;
    } else {if(!sync)fail("invalid NVN sync object",c->x[0]);copy_label(sync->label,c->x[1]);}
    pthread_mutex_unlock(&commands_lock);return 1;
}
static int event_import(Ctx *c,const char *name) {
    if(!strncmp(name,"nvnEventBuilder",15)) {
        const char *operation=name+15;uint64_t builder=c->x[0];
        if(!strcmp(operation,"SetDefaults")){for(unsigned i=0;i<32;i+=8)wr64(builder+i,0);return 1;}
        if(!strcmp(operation,"SetStorage")){wr64(builder,c->x[1]);wr64(builder+8,c->x[2]);return 1;}
        if(!strcmp(operation,"GetStorage")){if(c->x[1])wr64(c->x[1],rd64(builder+8));c->x[0]=rd64(builder);return 1;}
        if(!strcmp(operation,"GetMemoryPool")){c->x[0]=rd64(builder);return 1;}
        if(!strcmp(operation,"GetMemoryOffset")){c->x[0]=rd64(builder+8);return 1;}
        return 0;
    }
    if(strncmp(name,"nvnEvent",8))return 0;
    const char *operation=name+8;uint64_t event=c->x[0];
    if(!strcmp(operation,"Initialize")) {
        uint64_t pool=rd64(c->x[1]),offset=rd64(c->x[1]+8);
        for(unsigned i=0;i<64;i+=8)wr64(event+i,0);
        wr64(event,pool);wr64(event+8,offset);
        uint64_t address=pool?web_gpu_pool_address(pool,offset):0;
        if(!address){c->x[0]=0;return 1;}
        wr64(event+16,address);c->x[0]=1;return 1;
    }
    if(!strcmp(operation,"Finalize")){wr64(event+16,0);return 1;}
    if(!strcmp(operation,"GetMemoryPool")){c->x[0]=rd64(event);return 1;}
    if(!strcmp(operation,"GetMemoryOffset")){c->x[0]=rd64(event+8);return 1;}
    if(!strcmp(operation,"GetValue")){uint64_t address=rd64(event+16);c->x[0]=address?rd32(address):0;return 1;}
    if(!strcmp(operation,"Signal")) {
        uint64_t address=rd64(event+16);
        if(address){uint32_t value=(uint32_t)c->x[2];if((int)c->x[1]==1)value+=rd32(address);wr32(address,value);}
        return 1;
    }
    return 0;
}
static int window_import(Ctx *c,const char *name) {
    if(!strncmp(name,"nvnWindowBuilder",16)) {
        const char *operation=name+16;uint64_t builder=c->x[0];
        if(!strcmp(operation,"SetDefaults")){uint8_t dev=rd8(builder+6)==1?1:0;for(unsigned i=0;i<64;i+=8)wr64(builder+i,0);wr8(builder+4,1);wr8(builder+6,dev);return 1;}
        if(!strcmp(operation,"SetDevice")) {
            if(c->x[1]!=web_gpu_device_address())guest_trap("invalid window-builder device",c->x[1]);
            wr8(builder+6,1);return 1;
        }
        if(!strcmp(operation,"GetDevice")){c->x[0]=rd8(builder+6)?web_gpu_device_address():0;return 1;}
        if(!strcmp(operation,"SetNativeWindow")){wr64(builder+56,c->x[1]);return 1;}
        if(!strcmp(operation,"GetNativeWindow")){c->x[0]=rd64(builder+56);return 1;}
        if(!strcmp(operation,"SetPresentInterval")){wr8(builder+4,(uint8_t)c->x[1]);return 1;}
        if(!strcmp(operation,"GetPresentInterval")){c->x[0]=rd8(builder+4);return 1;}
        if(!strcmp(operation,"SetNumActiveTextures")){wr8(builder+5,(uint8_t)c->x[1]);return 1;}
        if(!strcmp(operation,"GetNumActiveTextures")){uint8_t active=rd8(builder+5);c->x[0]=active?active:(rd32(builder)&0xff);return 1;}
        if(!strcmp(operation,"SetTextures")) {
            unsigned count=c->x[1]<6?(unsigned)c->x[1]:6;wr8(builder,(uint8_t)count);
            for(unsigned i=0;i<count;i++)wr64(builder+8+(uint64_t)i*8,c->x[2]?rd64(c->x[2]+(uint64_t)i*8):0);
            return 1;
        }
        if(!strcmp(operation,"GetNumTextures")){c->x[0]=rd32(builder)&0xff;return 1;}
        if(!strcmp(operation,"GetTexture")){c->x[0]=c->x[1]<6?rd64(builder+8+c->x[1]*8):0;return 1;}
        return 0;
    }
    if(strncmp(name,"nvnWindow",9))return 0;
    const char *operation=name+9;
    const char *known_ops[]={"Initialize","Finalize","SetDebugLabel","AcquireTexture","SetPresentInterval","GetPresentInterval",
        "GetNativeWindow","GetNumTextures","GetNumActiveTextures","SetNumActiveTextures","SetCrop","GetCrop"};
    unsigned matched=0;for(unsigned i=0;i<sizeof known_ops/sizeof *known_ops;i++)if(!strcmp(operation,known_ops[i]))matched=1;
    if(!matched)return 0;
    pthread_mutex_lock(&commands_lock);Window **link=&windows;while(*link&&(*link)->key!=c->x[0])link=&(*link)->next;
    Window *win=*link;
    if(!strcmp(operation,"Initialize")) {
        uint64_t builder=c->x[1];
        int valid=c->x[0]&&!win&&builder&&rd8(builder+6)!=0;
        if(valid) {
            win=calloc(1,sizeof *win);
            if(win) {
                int count=(int)(rd32(builder)&0xff);
                win->key=c->x[0];win->count=count<=6?count:0;win->interval=rd8(builder+4);win->nactive=rd8(builder+5);win->native=rd64(builder+56);
                for(int i=0;i<win->count;i++)win->textures[i]=rd64(builder+8+(uint64_t)i*8);
                win->next=windows;windows=win;
            }
        }
        c->x[0]=valid&&win!=NULL;pthread_mutex_unlock(&commands_lock);return 1;
    }
    if(!win)fail("invalid NVN window",c->x[0]);
    if(!strcmp(operation,"Finalize")){*link=win->next;free(win);}
    else if(!strcmp(operation,"SetDebugLabel"))copy_label(win->label,c->x[1]);
    else if(!strcmp(operation,"AcquireTexture")) {
        int active=win->nactive?win->nactive:(win->count?win->count:2);
        uint32_t index=win->next_index++%(uint32_t)active;
        if(c->x[2])wr32(c->x[2],index);
        if(c->x[1]){Sync *sync=find_sync(c->x[1]);if(sync)sync->signaled=1;}
        c->x[0]=0;
    } else if(!strcmp(operation,"SetPresentInterval"))win->interval=(int)c->x[1];
    else if(!strcmp(operation,"GetPresentInterval"))c->x[0]=(uint32_t)win->interval;
    else if(!strcmp(operation,"GetNativeWindow"))c->x[0]=win->native;
    else if(!strcmp(operation,"GetNumTextures"))c->x[0]=(uint32_t)win->count;
    else if(!strcmp(operation,"GetNumActiveTextures"))c->x[0]=(uint32_t)(win->nactive?win->nactive:win->count);
    else if(!strcmp(operation,"SetNumActiveTextures")){if((int)c->x[1]>=1&&(int)c->x[1]<=win->count)win->nactive=(int)c->x[1];}
    else if(!strcmp(operation,"SetCrop")){for(unsigned i=0;i<4;i++)win->crop[i]=(int)c->x[1+i];}
    else if(!strcmp(operation,"GetCrop")) {
        int rect[4]={0,0,0,0};
        if(win->crop[2]>0&&win->crop[3]>0)memcpy(rect,win->crop,16);
        else if(win->count>0&&win->textures[0]) {
            WebGpuTextureRef *tex=web_gpu_texture_hold(win->textures[0]);
            if(tex){WebGpuTextureBuilder tb;uint32_t th;web_gpu_texture_snapshot(tex,&tb,&th);rect[2]=tb.width;rect[3]=tb.height;web_gpu_texture_release(tex);}
        }
        if(c->x[1])web_write(c->x[1],rect,16);
    }
    pthread_mutex_unlock(&commands_lock);return 1;
}
/* Selected command lists and the queue are retained. Only submit_lock is held;
 * the global registry lock is acquired briefly for descriptor/program state. */
static const char *execute_binding_chain(Queue *queue,PoolBinding *bindings) {
    for(PoolBinding *record=bindings;record;record=record->next) {
        if(record->sublist){const char *error=execute_binding_chain(queue,record->sublist->bindings);if(error)return error;}
        else if(record->render){const char *error=web_gpu_render_execute(&queue->render,record->render);if(error)return error;}
        else if(record->draw){
            uint32_t programs[6];uint64_t texture_pool,sampler_pool;pthread_mutex_lock(&commands_lock);
            for(unsigned i=0;i<6;i++)programs[i]=queue->programs[i]?queue->programs[i]->token:0;
            texture_pool=queue->texture_pool;sampler_pool=queue->sampler_pool;pthread_mutex_unlock(&commands_lock);
            const char *error=web_gpu_draw_execute(&queue->draw,record->draw,queue->render,programs,texture_pool,sampler_pool);if(error)return error;
        }
        else if(record->copy) {
            size_t bytes=web_gpu_copy_packet_size(record->copy);if(!bytes)continue;
            uint8_t *packet=malloc(4+bytes);if(!packet)return "ordered copy staging allocation failed";
            uint32_t one=1;memcpy(packet,&one,4);WebGpuCopyLease *lease=web_gpu_copy_prepare(record->copy,packet+4);
            if(!lease){free(packet);return "ordered copy source/texture staging failed";}
            WebGpuReply reply=web_gpu_request(17,0,0,4+bytes,packet,4+bytes);web_gpu_copy_release_lease(lease);free(packet);
            if(reply.status)return "ordered WebGPU copy submission failed";
        }else {
            const char *error=NULL;pthread_mutex_lock(&commands_lock);
            if(record->program_binding) {
                for(unsigned stage=0;stage<6;stage++)if(record->stages&(1u<<stage)) {
                    ProgramSnapshot *program=record->program;
                    if(queue->programs[stage]!=program){if(program)program->references++;release_program(queue->programs[stage]);queue->programs[stage]=program;}
                }
            }else {
                uint64_t *bound=record->sampler?&queue->sampler_pool:&queue->texture_pool;
                if(*bound!=record->pool){if(!web_gpu_descriptor_hold(record->pool,record->sampler))error="ordered descriptor retention failed";
                    else{if(*bound)web_gpu_descriptor_release(*bound,record->sampler);*bound=record->pool;}}
            }
            pthread_mutex_unlock(&commands_lock);if(error)return error;
        }
    }
    return NULL;
}
static const char *submit_ordered(Queue *queue,CommandList **selected,uint64_t count) {
    for(uint64_t i=0;i<count;i++){const char *error=execute_binding_chain(queue,selected[i]->bindings);if(error)return error;}
    return NULL;
}
/* Enter with commands_lock held and a retained queue. All exit paths drop
 * locks/references before reporting a failure. Other queues/guest callers can
 * progress while this queue stages bytes or waits for owner-Worker completion. */
static void submit_commands(Ctx *c,Queue *queue) {
    uint64_t count=c->x[1],array=c->x[2],key=queue->key;
    const char *error=NULL;CommandList **selected=NULL;uint64_t retained=0;
    uint8_t *packet=NULL;size_t length=4;uint32_t copies=0;
    WebGpuCopyLease **leases=NULL;uint32_t leased=0;
    int has_render=0;
    pthread_mutex_unlock(&commands_lock);pthread_mutex_lock(&queue->submit_lock);pthread_mutex_lock(&commands_lock);
    if(!queue->alive){error="submitting to a finalised NVN queue";goto cleanup_locked;}
    if(count>4096||(count&&(!array||array>UINT64_MAX-count*8))){error="invalid NVN submission handle array";goto cleanup_locked;}
    selected=calloc(count?count:1,sizeof *selected);
    if(!selected){error="submission handle allocation failed";goto cleanup_locked;}
    for(uint64_t i=0;i<count;i++) {
        uint64_t handle;
        if(gm_read(web_memory_current(),array+i*8,&handle,8)!=GM_OK){error="unmapped command handle array";goto cleanup_locked;}
        CommandList *list=lists;while(list&&list->handle!=handle)list=list->next;
        if(!list||list->references==UINT32_MAX){error="invalid or finalised command handle in submission";goto cleanup_locked;}
        list->references++;selected[retained++]=list;
        for(PoolBinding *record=list->bindings;record;record=record->next)if(record->render||record->draw||record->sublist)has_render=1;
        for(PoolBinding *record=list->bindings;record;record=record->next)if(record->copy) {
            size_t bytes=web_gpu_copy_packet_size(record->copy);
            if(bytes){if(bytes>128*1024*1024||length>128*1024*1024-bytes||copies==65536){error="copy submission exceeds staging limits";goto cleanup_locked;}length+=bytes;copies++;}
        }
    }
    pthread_mutex_unlock(&commands_lock);
    /* Mixed batches execute copies and clears in their recorded order. Keep
     * the existing copy-only batching path for startup texture uploads. */
    if(has_render) {
        error=submit_ordered(queue,selected,retained);pthread_mutex_lock(&commands_lock);
        if(!error)for(uint64_t i=0;i<retained;i++){queue->command_total+=selected[i]->command_bytes;queue->control_total+=32;}
        goto cleanup_locked;
    }
    packet=malloc(length);leases=calloc(copies?copies:1,sizeof *leases);
    if(!packet||!leases)error="copy submission staging allocation failed";
    else {
        memcpy(packet,&copies,4);size_t offset=4;
        for(uint64_t i=0;i<retained&&!error;i++)for(PoolBinding *record=selected[i]->bindings;record;record=record->next)if(record->copy) {
            size_t bytes=web_gpu_copy_packet_size(record->copy);
            if(bytes) {
                WebGpuCopyLease *lease=web_gpu_copy_prepare(record->copy,packet+offset);
                if(!lease){error="copy source/texture staging failed";break;}leases[leased++]=lease;
            }
            offset+=bytes;
        }
        if(!error&&web_gpu_request(17,0,0,length,packet,length).status)error="WebGPU copy submission failed";
    }
    pthread_mutex_lock(&commands_lock);
    if(!error)for(uint64_t i=0;i<retained;i++) {
        CommandList *list=selected[i];queue->command_total+=list->command_bytes;queue->control_total+=32;
        for(PoolBinding *record=list->bindings;record;record=record->next)if(record->program_binding) {
            for(unsigned stage=0;stage<6;stage++)if(record->stages&(1u<<stage)) {
                ProgramSnapshot *program=record->program;
                if(queue->programs[stage]!=program){
                    if(program)program->references++;
                    release_program(queue->programs[stage]);queue->programs[stage]=program;
                }
            }
        }else if(!record->copy) {
            uint64_t *bound=record->sampler?&queue->sampler_pool:&queue->texture_pool;
            if(*bound!=record->pool) {
                if(!web_gpu_descriptor_hold(record->pool,record->sampler)){error="queue descriptor retention failed";break;}
                if(*bound)web_gpu_descriptor_release(*bound,record->sampler);*bound=record->pool;
            }
        }
    }
cleanup_locked:
    for(uint32_t i=0;i<leased;i++)web_gpu_copy_release_lease(leases[i]);free(leases);
    for(uint64_t i=0;i<retained;i++)release_list(selected[i]);free(selected);free(packet);
    pthread_mutex_unlock(&commands_lock);pthread_mutex_unlock(&queue->submit_lock);
    pthread_mutex_lock(&commands_lock);release_queue(queue);pthread_mutex_unlock(&commands_lock);
    if(error)guest_trap(error,key);
}
static int queue_import(Ctx *c,const char *name) {
    if(!strncmp(name,"nvnQueueBuilder",15)) {
        const char *operation=name+15;uint64_t builder=c->x[0];
        if(!strcmp(operation,"SetDefaults")) {
            uint64_t dev=rd64(builder+56)==web_gpu_device_address()?web_gpu_device_address():0;
            for(unsigned i=0;i<64;i+=8)wr64(builder+i,0);
            wr64(builder+16,65536);wr64(builder+24,262144);wr64(builder+32,16384);wr32(builder+52,100);wr64(builder+56,dev);return 1;
        }
        const char *fields[]={"CommandMemorySize","ComputeMemorySize","ControlMemorySize","CommandFlushThreshold","Flags","QueuePriority","Device"};
        const unsigned offsets[]={16,24,32,40,48,52,56};
        int set=!strncmp(operation,"Set",3),get=!strncmp(operation,"Get",3);
        if(set||get)for(unsigned i=0;i<sizeof fields/sizeof *fields;i++)if(!strcmp(operation+3,fields[i])) {
            unsigned offset=offsets[i];
            if(set) {
                if(i==6&&c->x[1]!=web_gpu_device_address())guest_trap("invalid queue-builder device",c->x[1]);
                if(i==4||i==5)wr32(builder+offset,(uint32_t)c->x[1]);else wr64(builder+offset,c->x[1]);
            } else c->x[0]=(i==4||i==5)?rd32(builder+offset):rd64(builder+offset);
            return 1;
        }
        if(!strcmp(operation,"SetQueueMemory")){wr64(builder,c->x[1]);wr64(builder+8,c->x[2]);return 1;}
        if(!strcmp(operation,"GetQueueMemorySize")){c->x[0]=queue_memory_size(builder);return 1;}
        if(!strcmp(operation,"GetMemorySize")){c->x[0]=rd64(builder+8);return 1;}
        if(!strcmp(operation,"GetMemory")){c->x[0]=rd64(builder);return 1;}
        return 0;
    }
    if(strncmp(name,"nvnQueue",8))return 0;
    const char *operation=name+8;
    if(strcmp(operation,"Initialize")&&strcmp(operation,"Finalize")&&strcmp(operation,"Flush")&&strcmp(operation,"Finish")&&
       strcmp(operation,"GetError")&&strcmp(operation,"SetDebugLabel")&&strcmp(operation,"GetTotalCommandMemoryUsed")&&
       strcmp(operation,"GetTotalComputeMemoryUsed")&&strcmp(operation,"GetTotalControlMemoryUsed")&&strcmp(operation,"ResetMemoryUsageCounts")&&
       strcmp(operation,"FenceSync")&&strcmp(operation,"WaitSync")&&strcmp(operation,"SubmitCommands")&&
       strcmp(operation,"AcquireTexture")&&strcmp(operation,"PresentTexture"))return 0;
    pthread_mutex_lock(&commands_lock);Queue **link=&queues;while(*link&&(*link)->key!=c->x[0])link=&(*link)->next;
    Queue *queue=*link;
    if(!strcmp(operation,"Initialize")) {
        uint64_t builder=c->x[1],command=rd64(builder+16),compute=rd64(builder+24),control=rd64(builder+32);
        uint64_t memory=rd64(builder),size=rd64(builder+8),required=queue_memory_size(builder);uint8_t byte;
        int valid=c->x[0]&&!queue&&rd64(builder+56)==web_gpu_device_address()&&required&&command>=65536&&command%4096==0&&
            compute>=16384&&compute%256==0&&control>=4096&&control%4096==0;
        if(memory)valid=valid&&size>=required&&memory<=UINT64_MAX-size&&gm_read(web_memory_current(),memory,&byte,1)==GM_OK&&gm_read(web_memory_current(),memory+size-1,&byte,1)==GM_OK;
        if(valid) {
            queue=calloc(1,sizeof *queue);
            if(queue&&pthread_mutex_init(&queue->submit_lock,NULL)){free(queue);queue=NULL;}
            if(queue){queue->references=1;queue->alive=1;queue->key=c->x[0];queue->command_size=command;queue->compute_size=compute;queue->control_size=control;
                queue->flags=rd32(builder+48);queue->priority=rd32(builder+52);queue->next=queues;queues=queue;}
        }
        c->x[0]=valid&&queue!=NULL;pthread_mutex_unlock(&commands_lock);return 1;
    }
    if(!queue)fail("invalid NVN queue",c->x[0]);
    if(!strcmp(operation,"SubmitCommands")) {
        queue->references++;submit_commands(c,queue);return 1;
    } else if(!strcmp(operation,"AcquireTexture")) {
        Window *win=find_window(c->x[1]);
        if(!win)fail("invalid NVN window",c->x[1]);
        int active=win->nactive?win->nactive:(win->count?win->count:2);
        uint32_t index=win->next_index++%(uint32_t)active;
        if(c->x[2])wr32(c->x[2],index);
        c->x[0]=0;
    } else if(!strcmp(operation,"PresentTexture")) {
        Window *win=find_window(c->x[1]);
        if(!win)fail("invalid NVN window",c->x[1]);
        if(c->x[2]>=(uint64_t)win->count)fail("invalid NVN present texture index",c->x[2]);
        WebGpuTextureRef *texture=web_gpu_texture_hold(win->textures[c->x[2]]);
        if(!texture)fail("invalid NVN present texture",win->textures[c->x[2]]);
        WebGpuTextureBuilder builder;uint32_t handle;int32_t crop[4];
        web_gpu_texture_snapshot(texture,&builder,&handle);memcpy(crop,win->crop,sizeof crop);
        uint64_t key=queue->key;queue->references++;
        pthread_mutex_unlock(&commands_lock);
        pthread_mutex_lock(&queue->submit_lock);WebGpuReply reply=web_gpu_request(23,handle,0,0,crop,sizeof crop);pthread_mutex_unlock(&queue->submit_lock);
        web_gpu_texture_release(texture);
        pthread_mutex_lock(&commands_lock);release_queue(queue);pthread_mutex_unlock(&commands_lock);
        if(reply.status)guest_trap("WebGPU queue present failed",key);
        return 1;
    } else if(!strcmp(operation,"FenceSync")) {
        Sync *sync=find_sync(c->x[1]);
        if(!sync||c->x[2]>1||(c->x[3]&~1ULL))fail("invalid NVN fence parameters",c->x[1]);
        Fence *fence=calloc(1,sizeof *fence);
        if(!fence)fail("NVN fence allocation failed",sync->key);
        WebGpuReply reply=web_gpu_request(8,0,0,0,NULL,0);
        if(reply.status){free(fence);fail("GPU fence snapshot failed",sync->key);}
        fence->handle=reply.value;fence->references=1;release_fence(sync->fence);sync->fence=fence;sync->signaled=1;
    } else if(!strcmp(operation,"WaitSync")) {
        Sync *sync=find_sync(c->x[1]);Fence *fence=sync?sync->fence:NULL;
        if(!fence){c->x[0]=(sync&&sync->signaled)?1:0;pthread_mutex_unlock(&commands_lock);return 1;}
        fence->references++;
        pthread_mutex_unlock(&commands_lock);
        WebGpuReply reply=web_gpu_request(11,fence->handle,0,0,NULL,0);
        pthread_mutex_lock(&commands_lock);release_fence(fence);c->x[0]=!reply.status&&reply.value!=0;
    } else if(!strcmp(operation,"Finish")||!strcmp(operation,"Finalize")||!strcmp(operation,"Flush")) {
        uint64_t key=queue->key;
        unsigned gpu_operation=!strcmp(operation,"Flush")?5:6;queue->references++;
        if(!strcmp(operation,"Finalize")){*link=queue->next;queue->alive=0;release_queue(queue);}
        pthread_mutex_unlock(&commands_lock);
        pthread_mutex_lock(&queue->submit_lock);WebGpuReply reply=web_gpu_request(gpu_operation,0,0,0,NULL,0);pthread_mutex_unlock(&queue->submit_lock);
        pthread_mutex_lock(&commands_lock);release_queue(queue);pthread_mutex_unlock(&commands_lock);
        if(reply.status)guest_trap("WebGPU queue completion failed",key);
        return 1;
    } else if(!strcmp(operation,"GetError")) {
        WebGpuReply reply=web_gpu_request(7,0,0,0,NULL,0);c->x[0]=reply.status?1:reply.value;
    } else if(!strcmp(operation,"SetDebugLabel"))copy_label(queue->label,c->x[1]);
    else if(!strncmp(operation,"GetTotal",8)) {
        c->x[0]=!strcmp(operation,"GetTotalCommandMemoryUsed")?queue->command_total:!strcmp(operation,"GetTotalControlMemoryUsed")?queue->control_total:0;
    } else if(!strcmp(operation,"ResetMemoryUsageCounts"))queue->command_total=queue->control_total=0;
    pthread_mutex_unlock(&commands_lock);return 1;
}
int web_gpu_commands_import(Ctx *c,const char *name) {
    if(sync_import(c,name))return 1;
    if(event_import(c,name))return 1;
    if(window_import(c,name))return 1;
    if(queue_import(c,name))return 1;
    if(!strcmp(name,"nvnDeviceFinalizeCommandHandle")) {
        pthread_mutex_lock(&commands_lock);
        if(c->x[0]!=web_gpu_device_address())fail("invalid NVN command-list device",c->x[0]);
        CommandList **link=&lists;while(*link&&(*link)->handle!=c->x[1])link=&(*link)->next;
        if(!*link)fail("invalid or finalised NVN command handle",c->x[1]);
        CommandList *list=*link;*link=list->next;release_list(list);
        pthread_mutex_unlock(&commands_lock);return 1;
    }
    if(strncmp(name,"nvnCommandBuffer",16)||!known(name+16))return 0;
    const char *operation=name+16;
    pthread_mutex_lock(&commands_lock);CommandBuffer *buffer=find(c->x[0]);
    if(!strcmp(operation,"Initialize")) {
        if(!c->x[0]||buffer||c->x[1]!=web_gpu_device_address()){c->x[0]=0;pthread_mutex_unlock(&commands_lock);return 1;}
        buffer=calloc(1,sizeof *buffer);
        if(buffer){buffer->key=c->x[0];buffer->callback_enabled=1;buffer->next=buffers[bucket(buffer->key)];buffers[bucket(buffer->key)]=buffer;}
        c->x[0]=buffer!=NULL;pthread_mutex_unlock(&commands_lock);return 1;
    }
    if(!buffer)fail("invalid NVN command buffer",c->x[0]);
    if(!strcmp(operation,"Finalize")) {
        if(buffer->in_callback)fail("finalising command buffer during refill",buffer->key);
        if(buffer->recording)fail("finalising a recording command buffer",buffer->key);
        free_bindings(buffer->bindings);
        if(buffer->pool)web_gpu_pool_release(buffer->pool);
        CommandBuffer **link=&buffers[bucket(buffer->key)];while(*link!=buffer)link=&(*link)->next;
        *link=buffer->next;free(buffer);
    } else if(!strcmp(operation,"AddCommandMemory")) {
        if(c->x[2]%4||c->x[3]%4||!web_gpu_pool_hold(c->x[1],c->x[2],c->x[3]))fail("invalid NVN command memory range",c->x[1]);
        if(buffer->pool)web_gpu_pool_release(buffer->pool);
        buffer->pool=c->x[1];buffer->command_size=c->x[3];buffer->command_used=0;
    } else if(!strcmp(operation,"AddControlMemory")) {
        uint8_t byte;uint64_t address=c->x[1],size=c->x[2];
        if(!address||address%8||size<32||address>UINT64_MAX-size||gm_read(web_memory_current(),address,&byte,1)!=GM_OK||
           gm_read(web_memory_current(),address+size-1,&byte,1)!=GM_OK)fail("invalid NVN control memory range",address);
        buffer->control=address;buffer->control_size=size;buffer->control_used=0;
    } else if(!strcmp(operation,"BeginRecording")) {
        if(buffer->in_callback)fail("starting command recording during refill",buffer->key);
        if(buffer->recording)fail("command buffer already recording",buffer->key);
        buffer->recording=1;buffer->record_bytes=0;
    } else if(!strcmp(operation,"EndRecording")) {
        if(buffer->in_callback)fail("ending command recording during refill",buffer->key);
        if(!buffer->recording)fail("command buffer is not recording",buffer->key);
        require_memory(c,buffer,32,1);
        CommandList *list=calloc(1,sizeof *list);
        if(!list||!next_list){free(list);fail("NVN command handle allocation failed",buffer->key);}
        list->references=1;list->command_bytes=buffer->record_bytes;
        list->bindings=buffer->bindings;buffer->bindings=buffer->bindings_tail=NULL;
        list->handle=0x7c00000000ULL+(uint64_t)next_list++*16;list->next=lists;lists=list;
        buffer->control_used+=32;buffer->recording=0;c->x[0]=list->handle;
    } else if(!strcmp(operation,"SetTexturePool")||!strcmp(operation,"SetSamplerPool")) {
        if(!buffer->recording)fail("pool binding outside command recording",buffer->key);
        require_memory(c,buffer,16,0);
        PoolBinding *binding=calloc(1,sizeof *binding);
        if(!binding)fail("command pool-binding allocation failed",buffer->key);
        binding->pool=c->x[1];binding->sampler=!strcmp(operation,"SetSamplerPool");
        if(!web_gpu_descriptor_hold(binding->pool,binding->sampler)){free(binding);fail("invalid command descriptor pool",c->x[1]);}
        if(buffer->bindings_tail)buffer->bindings_tail->next=binding;else buffer->bindings=binding;
        buffer->bindings_tail=binding;buffer->command_used+=16;buffer->record_bytes+=16;
    } else if(web_gpu_draw_known(operation)) {
        if(!buffer->recording)fail("graphics command outside command recording",buffer->key);
        require_memory(c,buffer,32,0);
        PoolBinding *record=calloc(1,sizeof *record);if(!record)fail("graphics command allocation failed",buffer->key);
        const char *error=NULL;record->draw=web_gpu_draw_record(c,operation,&error);
        if(!record->draw){free(record);fail(error,buffer->key);}
        if(buffer->bindings_tail)buffer->bindings_tail->next=record;else buffer->bindings=record;
        buffer->bindings_tail=record;buffer->command_used+=32;buffer->record_bytes+=32;
    } else if(web_gpu_render_known(operation)) {
        if(!buffer->recording)fail("render command outside command recording",buffer->key);
        uint64_t bytes=!strcmp(operation,"SetRenderTargets")?320:!strcmp(operation,"SetScissors")&&c->x[2]<=16?16+c->x[2]*16:32;
        require_memory(c,buffer,bytes,0);
        PoolBinding *record=calloc(1,sizeof *record);if(!record)fail("render command allocation failed",buffer->key);
        const char *error=NULL;record->render=web_gpu_render_record(c,operation,&error);
        if(!record->render){free(record);fail(error,buffer->key);}
        if(buffer->bindings_tail)buffer->bindings_tail->next=record;else buffer->bindings=record;
        buffer->bindings_tail=record;buffer->command_used+=bytes;buffer->record_bytes+=bytes;
    } else if(!strcmp(operation,"BindProgram")) {
        if(!buffer->recording)fail("program binding outside command recording",buffer->key);
        if(c->x[2]&~63ULL)fail("invalid NVN shader stage mask",c->x[2]);
        require_memory(c,buffer,16,0);
        PoolBinding *record=calloc(1,sizeof *record);if(!record)fail("program binding allocation failed",buffer->key);
        record->program_binding=1;record->stages=(uint32_t)c->x[2];
        if(c->x[1]&&record->stages) {
            record->program=calloc(1,sizeof *record->program);
            if(!record->program){free(record);fail("program snapshot allocation failed",buffer->key);}
            if(!web_gpu_program_snapshot(c->x[1],&record->program->token)){
                free(record->program);free(record);fail("invalid NVN program for command binding",c->x[1]);
            }
            record->program->references=1;
        }
        if(buffer->bindings_tail)buffer->bindings_tail->next=record;else buffer->bindings=record;
        buffer->bindings_tail=record;buffer->command_used+=16;buffer->record_bytes+=16;
    } else if(!strcmp(operation,"CopyBufferToTexture")) {
        if(!buffer->recording)fail("copy outside command recording",buffer->key);
        require_memory(c,buffer,128,0);
        PoolBinding *record=calloc(1,sizeof *record);if(!record)fail("copy command allocation failed",buffer->key);
        const char *error=NULL;record->copy=web_gpu_copy_record(c,buffer->row_stride,buffer->image_stride,&error);
        if(!record->copy){free(record);fail(error,buffer->key);}
        if(buffer->bindings_tail)buffer->bindings_tail->next=record;else buffer->bindings=record;
        buffer->bindings_tail=record;buffer->command_used+=128;buffer->record_bytes+=128;
    } else if(!strcmp(operation,"SetCopyRowStride"))buffer->row_stride=c->x[1];
    else if(!strcmp(operation,"GetCopyRowStride"))c->x[0]=buffer->row_stride;
    else if(!strcmp(operation,"SetCopyImageStride"))buffer->image_stride=c->x[1];
    else if(!strcmp(operation,"GetCopyImageStride"))c->x[0]=buffer->image_stride;
    else if(!strcmp(operation,"GetCommandMemorySize"))c->x[0]=buffer->command_size;
    else if(!strcmp(operation,"GetControlMemorySize"))c->x[0]=buffer->control_size;
    else if(!strcmp(operation,"GetCommandMemoryUsed"))c->x[0]=buffer->command_used;
    else if(!strcmp(operation,"GetControlMemoryUsed"))c->x[0]=buffer->control_used;
    else if(!strcmp(operation,"GetCommandMemoryFree"))c->x[0]=buffer->command_size-buffer->command_used;
    else if(!strcmp(operation,"GetControlMemoryFree"))c->x[0]=buffer->control_size-buffer->control_used;
    else if(!strcmp(operation,"IsRecording"))c->x[0]=buffer->recording;
    else if(!strcmp(operation,"SetMemoryCallback"))buffer->callback=c->x[1];
    else if(!strcmp(operation,"SetMemoryCallbackData"))buffer->callback_data=c->x[1];
    else if(!strcmp(operation,"GetMemoryCallback"))c->x[0]=buffer->callback;
    else if(!strcmp(operation,"GetMemoryCallbackData"))c->x[0]=buffer->callback_data;
    else if(!strcmp(operation,"SetCommandMemoryCallbackEnabled"))buffer->callback_enabled=c->x[1]&1;
    else if(!strcmp(operation,"SetDebugLabel"))copy_label(buffer->label,c->x[1]);
    else if(!strcmp(operation,"SignalEvent")) {
        uint64_t address=c->x[1]?rd64(c->x[1]+16):0;
        if(address){uint32_t value=(uint32_t)c->x[3];if((int)c->x[2]==1)value+=rd32(address);wr32(address,value);}
    } else if(!strcmp(operation,"FenceSync")) {
        if(c->x[1]){Sync *sync=find_sync(c->x[1]);if(sync)sync->signaled=1;}
    } else if(!strcmp(operation,"CallCommands")||!strcmp(operation,"CopyCommands")) {
        uint64_t count=c->x[1],array=c->x[2];
        if(buffer->recording&&count<=4096&&array) {
            for(uint64_t i=0;i<count;i++) {
                uint64_t handle=0;
                if(gm_read(web_memory_current(),array+i*8,&handle,8)!=GM_OK)break;
                CommandList *sub=lists;while(sub&&sub->handle!=handle)sub=sub->next;
                if(sub&&sub->bindings) {
                    PoolBinding *record=calloc(1,sizeof *record);
                    if(record){sub->references++;record->sublist=sub;if(buffer->bindings_tail)buffer->bindings_tail->next=record;else buffer->bindings=record;buffer->bindings_tail=record;}
                }
            }
        }
    } else if(!strcmp(operation,"ReportCounter")||!strcmp(operation,"ReportValue")) {
        uint64_t dst=c->x[2];uint8_t byte;
        if(dst&&gm_read(web_memory_current(),dst,&byte,1)==GM_OK&&gm_read(web_memory_current(),dst+15,&byte,1)==GM_OK) {
            uint64_t val=!strcmp(operation,"ReportValue")?(uint32_t)c->x[1]:0;
            uint64_t ts=(uint64_t)(emscripten_get_now()*1000000.0);
            wr64(dst,val);wr64(dst+8,ts);
        }
    } else if(!strcmp(operation,"ClearBuffer")) {
        uint64_t dst=c->x[1],size=c->x[2];uint32_t val=(uint32_t)c->x[3];uint8_t byte;
        if(dst&&size&&size<=64*1024*1024&&(size%4)==0&&gm_read(web_memory_current(),dst,&byte,1)==GM_OK&&gm_read(web_memory_current(),dst+size-1,&byte,1)==GM_OK) {
            for(uint64_t off=0;off<size;off+=4)wr32(dst+off,val);
        }
    } else if(!strcmp(operation,"CopyBufferToBuffer")) {
        uint64_t src=c->x[1],dst=c->x[2],size=c->x[3];uint8_t byte;
        if(src&&dst&&size&&size<=64*1024*1024&&gm_read(web_memory_current(),src,&byte,1)==GM_OK&&gm_read(web_memory_current(),src+size-1,&byte,1)==GM_OK&&
           gm_read(web_memory_current(),dst,&byte,1)==GM_OK&&gm_read(web_memory_current(),dst+size-1,&byte,1)==GM_OK) {
            for(uint64_t off=0;off<size;off++)wr8(dst+off,rd8(src+off));
        }
    } else if(!strcmp(operation,"UpdateUniformBuffer")) {
        uint64_t dst=c->x[1]+c->x[3],src=c->x[5],size=c->x[4];uint8_t byte;
        if(c->x[1]&&src&&size&&size<=65536&&gm_read(web_memory_current(),dst,&byte,1)==GM_OK&&gm_read(web_memory_current(),src,&byte,1)==GM_OK) {
            for(uint64_t i=0;i<size;i++)wr8(dst+i,rd8(src+i));
        }
    }
    pthread_mutex_unlock(&commands_lock);return 1;
}
