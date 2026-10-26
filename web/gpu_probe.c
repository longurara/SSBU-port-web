/* Self-authored integration fixture through the same NVN entry points and
 * pthread -> owner-Worker -> WebGPU bridge used by the game runtime. */
#include "runtime.h"
#include "host_gpu_bridge.h"
#include "host_gpu_sampler.h"
#include "host_gpu_texture.h"
#include "tex_decode.h"
#include <emscripten/emscripten.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

int web_gpu_import(Ctx *,const char *);
int web_gpu_indirect(Ctx *,uint64_t);
static jmp_buf stopped;
static jmp_buf expected_stop;
static int trap_expected;
static jmp_buf *callback_boundary;
static unsigned callback_calls,callback_last_event,callback_last_minimum;
static int callback_mode;static char callback_fault[160];
static const uint64_t REFILL_CALLBACK=0x7100123456ULL,REFILL_COMMANDS=0x7100006a00ULL;
static void refill_callback(Ctx *c);
static unsigned checks,failures,first_failure_line;
static unsigned last_texture_format;static uint64_t last_texture_storage;
static char error[160],report[512];
static pthread_t driver;
static GuestMemory memory;
static Ctx context;
__attribute__((noinline)) static int graphics_fixture(void){return MAIN_THREAD_EM_ASM_INT({return Module['graphicsFixture'] ? 1 : 0;});}
static uint8_t expected[65536],readback[65536];
static const uint64_t BASE=0x7100000000ULL,DEVICE=0x7100001000ULL,DEVICE_BUILDER=0x7100005000ULL;
static const uint64_t POOL_BUILDER=0x7100005100ULL,BUFFER_BUILDER=0x7100005200ULL,COPIED_BUILDER=0x7100005300ULL;
static const uint64_t POOL_OBJECT=0x7100006000ULL,BUFFER_OBJECT=0x7100006200ULL,BAD_BUFFER=0x7100006300ULL;
static const uint64_t TEXTURES=0x7100006400ULL,SAMPLERS=0x7100006500ULL;
static const uint64_t COMMANDS=0x7100006800ULL,CONTROL=0x7100009000ULL;
static const uint64_t QUEUE_BUILDER=0x7100007000ULL,QUEUE=0x710000a000ULL;
static const uint64_t SYNC=0x7100006600ULL;
static const uint64_t STATE=0x710000eff8ULL,STATE_COPY=0x710000f100ULL;
static const uint64_t SAMPLER_BUILDER=0x7100007200ULL,SAMPLER=0x7100006700ULL;
static const uint64_t NAME=0x7100008000ULL,WORKSPACE=0x7100010000ULL,STORAGE=0x7100020000ULL;
void guest_trap(const char *what,uint64_t address) {
    snprintf(error,sizeof error,"%s (%llx)",what,(unsigned long long)address);
    if(callback_boundary)longjmp(*callback_boundary,1);
    if(trap_expected)longjmp(expected_stop,1);
    longjmp(stopped,1);
}
void call_indirect(Ctx *c,uint64_t function){if(function!=REFILL_CALLBACK)guest_trap("unexpected GPU fixture callback",function);refill_callback(c);}
int web_runtime_thread_call(Ctx *c,uint64_t function){
    jmp_buf boundary;jmp_buf *previous=callback_boundary;callback_boundary=&boundary;int ok=0;
    if(!setjmp(boundary)){call_indirect(c,function);ok=1;}else snprintf(callback_fault,sizeof callback_fault,"%s",error);
    callback_boundary=previous;return ok;
}
/* This fixture stays dump-free. Real SDK sizing is tested independently by
 * check_nvn_layout.py with the same host_nvn_layout.c used in the game. */
#ifndef WEB_GPU_WITH_SDK
uint64_t web_nvn_layout_query(Ctx *c,const WebGpuTextureBuilder *b,unsigned function,uint32_t output[4]) {
    (void)c;(void)b;(void)output;guest_trap("SDK layout image is not part of the dump-free GPU fixture",function);return 0;
}
#else
void guest_unimpl(uint64_t address,uint32_t instruction,const char *what){(void)instruction;guest_trap(what,address);}
#endif
static void check_at(int value,unsigned line) {checks++;if(!value){failures++;if(!first_failure_line)first_failure_line=line;}}
#define check(value) check_at((value),__LINE__)
static void texture_source(uint8_t *bytes,size_t size,int format,int mip,int layer) {
    for(size_t i=0;i<size;i++)bytes[i]=(uint8_t)(i*73+mip*11+layer*97+5);
    if(format==0x22)for(size_t i=0;i<size;i+=4){uint32_t bits=0x3f000000+(uint32_t)i*1024;memcpy(bytes+i,&bits,4);}
    if(format==0x34||format==0x36)for(size_t i=0;i<size;i+=(format==0x36?8:4)){
        float value=(float[]){0,0.125f,0.5f,0.75f,1,0.9f}[(i/4+(size_t)mip+(size_t)layer)%6];memcpy(bytes+i,&value,4);
        if(format==0x36){uint32_t stencil=(uint32_t)(i+17)&255;memcpy(bytes+i+4,&stencil,4);}
    }
}
static uint64_t invoke(const char *name,uint64_t a,uint64_t b,uint64_t d,uint64_t e) {
    web_write(NAME,name,strlen(name)+1);context.x[0]=NAME;
    if(!web_gpu_import(&context,"nvnBootstrapLoader"))guest_trap("bootstrap not handled",0);
    uint64_t function=context.x[0];context.x[0]=a;context.x[1]=b;context.x[2]=d;context.x[3]=e;
    if(!web_gpu_indirect(&context,function))guest_trap("entry point not handled",function);
    return context.x[0];
}
static void refill_callback(Ctx *c){
    callback_calls++;callback_last_event=(unsigned)c->x[1];callback_last_minimum=(unsigned)c->x[2];
    const uint64_t key=c->x[0],minimum=c->x[2],event=c->x[1];
    check(c==&context&&key==REFILL_COMMANDS&&event<=1&&c->x[3]==44&&minimum>=256&&!(minimum&255));
    check(invoke("nvnCommandBufferIsRecording",key,0,0,0)==1); // Reentrant call must not deadlock.
    if(callback_mode==2)invoke("nvnCommandBufferSetTexturePool",key,TEXTURES,0,0);
    if(callback_mode==3)invoke("nvnCommandBufferFinalize",key,0,0,0);
    /* The refill-retention test supplies distinct storage. Reusing an old
     * slot deliberately invalidates its registry handle (tested separately). */
    if(!callback_mode){if(event)invoke("nvnCommandBufferAddControlMemory",key,BASE+0x3c000+callback_calls*512,minimum,0);else invoke("nvnCommandBufferAddCommandMemory",key,POOL_OBJECT,0,minimum);}
    for(unsigned i=0;i<19;i++)c->x[i]=0xdead0000u+i;
    memset(c->v,0,8*sizeof c->v[0]);c->fn=0xdeadbad;
}
static void control_reuse_checks(void) {
    const uint64_t a=BASE+0x6c00,b=BASE+0x6e00,q=BASE+0x6f00,control=CONTROL+3072,handles=CONTROL+4000;
    int sampler=-1;uint64_t pool=0;
    check(invoke("nvnCommandBufferInitialize",a,DEVICE,0,0)==1);
    check(invoke("nvnCommandBufferInitialize",b,DEVICE,0,0)==1);
    invoke("nvnCommandBufferAddCommandMemory",a,POOL_OBJECT,0,4096);
    invoke("nvnCommandBufferAddCommandMemory",b,POOL_OBJECT,0,4096);
    invoke("nvnCommandBufferAddControlMemory",a,control,128,0);
    invoke("nvnCommandBufferBeginRecording",a,0,0,0);
    invoke("nvnCommandBufferSetTexturePool",a,TEXTURES,0,0);
    uint64_t first=invoke("nvnCommandBufferEndRecording",a,0,0,0);
    check(web_gpu_command_pool_binding(first,0,&sampler,&pool)&&!sampler&&pool==TEXTURES);
    invoke("nvnCommandBufferAddControlMemory",a,control,128,0);
    check(web_gpu_command_pool_binding(first,0,&sampler,&pool)); // Declaring storage alone does not overwrite.
    invoke("nvnCommandBufferBeginRecording",a,0,0,0);
    invoke("nvnCommandBufferSetSamplerPool",a,SAMPLERS,0,0);
    uint64_t second=invoke("nvnCommandBufferEndRecording",a,0,0,0);
    check(second!=first&&!web_gpu_command_pool_binding(first,0,&sampler,&pool));
    check(web_gpu_command_pool_binding(second,0,&sampler,&pool)&&sampler&&pool==SAMPLERS);
    wr64(handles,first);
    if(!setjmp(expected_stop)){trap_expected=1;invoke("nvnQueueSubmitCommands",QUEUE,1,handles,0);trap_expected=0;check(0);}
    else{trap_expected=0;check(strstr(error,"invalid or finalised command handle")!=NULL);error[0]=0;}
    invoke("nvnCommandBufferBeginRecording",a,0,0,0);
    invoke("nvnCommandBufferSetTexturePool",a,TEXTURES,0,0);
    uint64_t third=invoke("nvnCommandBufferEndRecording",a,0,0,0);
    check(web_gpu_command_pool_binding(second,0,&sampler,&pool)); // Adjacent 32-byte slots remain independent.
    check(web_gpu_command_pool_binding(third,0,&sampler,&pool));
    invoke("nvnCommandBufferAddControlMemory",b,control+24,128,0);
    check(web_gpu_command_pool_binding(second,0,&sampler,&pool)&&web_gpu_command_pool_binding(third,0,&sampler,&pool));
    invoke("nvnCommandBufferBeginRecording",b,0,0,0);
    invoke("nvnCommandBufferSetTexturePool",b,TEXTURES,0,0);
    uint64_t overlap=invoke("nvnCommandBufferEndRecording",b,0,0,0);
    check(!web_gpu_command_pool_binding(second,0,&sampler,&pool)&&!web_gpu_command_pool_binding(third,0,&sampler,&pool));
    check(web_gpu_command_pool_binding(overlap,0,&sampler,&pool));
    invoke("nvnDeviceFinalizeCommandHandle",DEVICE,overlap,0,0);
    uint64_t previous=0;
    for(unsigned i=0;i<96;i++){
        invoke("nvnCommandBufferAddControlMemory",a,control+128,128,0);
        invoke("nvnCommandBufferBeginRecording",a,0,0,0);
        invoke(i&1?"nvnCommandBufferSetSamplerPool":"nvnCommandBufferSetTexturePool",a,i&1?SAMPLERS:TEXTURES,0,0);
        uint64_t current=invoke("nvnCommandBufferEndRecording",a,0,0,0);
        check(current!=previous&&(!previous||!web_gpu_command_pool_binding(previous,0,&sampler,&pool)));
        check(web_gpu_command_pool_binding(current,0,&sampler,&pool)&&sampler==(int)(i&1)&&pool==(i&1?SAMPLERS:TEXTURES));previous=current;
    }
    invoke("nvnDeviceFinalizeCommandHandle",DEVICE,previous,0,0);
    invoke("nvnCommandBufferAddControlMemory",a,control+384,128,0);
    invoke("nvnCommandBufferBeginRecording",a,0,0,0);invoke("nvnCommandBufferSetTexturePool",a,TEXTURES,0,0);
    uint64_t child=invoke("nvnCommandBufferEndRecording",a,0,0,0);
    wr64(handles,child);invoke("nvnCommandBufferAddControlMemory",b,control+512,128,0);
    invoke("nvnCommandBufferBeginRecording",b,0,0,0);invoke("nvnCommandBufferCallCommands",b,1,handles,0);
    uint64_t parent=invoke("nvnCommandBufferEndRecording",b,0,0,0);
    invoke("nvnCommandBufferAddControlMemory",a,control+384,128,0);
    invoke("nvnCommandBufferBeginRecording",a,0,0,0);
    uint64_t replacement=invoke("nvnCommandBufferEndRecording",a,0,0,0);
    check(!web_gpu_command_pool_binding(child,0,&sampler,&pool));
    if(!setjmp(expected_stop)){trap_expected=1;invoke("nvnTexturePoolFinalize",TEXTURES,0,0,0);trap_expected=0;check(0);}
    else{trap_expected=0;check(strstr(error,"referenced by recorded commands")!=NULL);error[0]=0;}
    check(invoke("nvnQueueInitialize",q,QUEUE_BUILDER,0,0)==1);
    wr64(handles,parent);invoke("nvnQueueSubmitCommands",q,1,handles,0); // Parent keeps the evicted child's metadata alive.
    invoke("nvnDeviceFinalizeCommandHandle",DEVICE,parent,0,0);
    invoke("nvnQueueFinalize",q,0,0,0);
    invoke("nvnDeviceFinalizeCommandHandle",DEVICE,replacement,0,0);
    invoke("nvnCommandBufferFinalize",a,0,0,0);invoke("nvnCommandBufferFinalize",b,0,0,0);
}
__attribute__((noinline)) static void publish(void) {
    MAIN_THREAD_ASYNC_EM_ASM({Module['onGpuProbeReport'](UTF8ToString($0));},report);
}
static void *run(void *unused) {
    (void)unused;
#ifdef WEB_GPU_WITH_SDK
    gm_init_limit(&memory,4096);
#else
    gm_init(&memory);
#endif
    if(!setjmp(stopped)) {
        check(gm_reserve(&memory,BASE,1048576)==GM_OK&&gm_commit(&memory,BASE,262144)==GM_OK);
        web_memory_bind(&memory);
        context.x[31]=BASE+0x3fff0;
        context.x[0]=WORKSPACE;context.x[1]=65536;
        check(web_gpu_import(&context,"_ZN2nv18InitializeGraphicsEPvm"));
        invoke("nvnDeviceBuilderSetDefaults",DEVICE_BUILDER,0,0,0);
        check(invoke("nvnDeviceInitialize",DEVICE,DEVICE_BUILDER,0,0)==1);
        uint64_t packed_handle=invoke("nvnDeviceGetTextureHandle",DEVICE,0x12345,4095,0);
        check((packed_handle&0xfffff)==0x12345&&((packed_handle>>20)&0xfff)==4095);
        check(invoke("nvnDeviceGetTexelFetchHandle",DEVICE,123,0,0)==UINT64_C(0x10000007b));
        check(invoke("nvnDeviceGetSeparateTextureHandle",DEVICE,123,0,0)==UINT64_C(0x10000007b));
        check(invoke("nvnDeviceGetSeparateSamplerHandle",DEVICE,123,0,0)==(UINT64_C(123)<<20));
        check(invoke("nvnDeviceGetImageHandle",DEVICE,123,0,0)==123);
        // A 24-byte state crossing a page boundary must not use a borrowed
        // single-page host pointer. Copies retain independent guest state.
        invoke("nvnMultisampleStateSetDefaults",STATE,0,0,0);
        invoke("nvnMultisampleStateSetSamples",STATE,4,0,0);
        uint8_t state_copy[24];web_read(STATE,state_copy,24);web_write(STATE_COPY,state_copy,24);
        invoke("nvnMultisampleStateSetSamples",STATE,8,0,0);
        check(invoke("nvnMultisampleStateGetSamples",STATE_COPY,0,0,0)==4);
        check(invoke("nvnMultisampleStateGetSamples",STATE,0,0,0)==8);
        invoke("nvnChannelMaskStateSetDefaults",STATE_COPY,0,0,0);
        context.x[4]=1;context.x[5]=0;
        invoke("nvnChannelMaskStateSetChannelMask",STATE_COPY,3,1,0);
        uint8_t boolean_bytes[8];memset(boolean_bytes,0xa5,8);web_write(STATE_COPY+32,boolean_bytes,8);
        context.x[4]=STATE_COPY+36;context.x[5]=STATE_COPY+38;
        invoke("nvnChannelMaskStateGetChannelMask",STATE_COPY,3,STATE_COPY+32,STATE_COPY+34);
        context.x[4]=context.x[5]=0;web_read(STATE_COPY+32,boolean_bytes,8);
        static const uint8_t expected_booleans[8]={1,0xa5,0,0xa5,1,0xa5,0,0xa5};
        check(!memcmp(boolean_bytes,expected_booleans,8));
        invoke("nvnVertexAttribStateSetDefaults",STATE_COPY,0,0,0);
        invoke("nvnVertexAttribStateSetStreamIndex",STATE_COPY,7,0,0);
        invoke("nvnVertexAttribStateSetFormat",STATE_COPY,0x2d,0x12345,0);
        wr64(STATE_COPY+64,UINT64_MAX);
        invoke("nvnVertexAttribStateGetFormat",STATE_COPY,STATE_COPY+48,STATE_COPY+64,0);
        check(rd64(STATE_COPY+64)==0x12345);
        check(invoke("nvnVertexAttribStateGetStreamIndex",STATE_COPY,0,0,0)==7);
        for(unsigned i=0;i<sizeof expected;i++)expected[i]=(uint8_t)(i*13+7);
        web_write(STORAGE,expected,sizeof expected);
        invoke("nvnMemoryPoolBuilderSetDevice",POOL_BUILDER,DEVICE,0,0);
        invoke("nvnMemoryPoolBuilderSetDefaults",POOL_BUILDER,0,0,0);
        check(invoke("nvnMemoryPoolBuilderGetDevice",POOL_BUILDER,0,0,0)==DEVICE);
        invoke("nvnMemoryPoolBuilderSetStorage",POOL_BUILDER,STORAGE,81920,0);
        invoke("nvnMemoryPoolBuilderSetFlags",POOL_BUILDER,4,0,0);
        check(invoke("nvnMemoryPoolInitialize",POOL_OBJECT,POOL_BUILDER,0,0)==1);
        check(invoke("nvnMemoryPoolGetSize",POOL_OBJECT,0,0,0)==81920);
        check(invoke("nvnMemoryPoolMap",POOL_OBJECT,0,0,0)==STORAGE);
        check(invoke("nvnTexturePoolInitialize",TEXTURES,POOL_OBJECT,0,256)==1);
        check(invoke("nvnSamplerPoolInitialize",SAMPLERS,POOL_OBJECT,8192,512)==1);
        check(invoke("nvnTexturePoolGetMemoryPool",TEXTURES,0,0,0)==POOL_OBJECT);
        check(invoke("nvnSamplerPoolGetMemoryOffset",SAMPLERS,0,0,0)==8192);
        check(invoke("nvnTexturePoolGetSize",TEXTURES,0,0,0)==256);
        invoke("nvnSamplerBuilderSetDevice",SAMPLER_BUILDER,DEVICE,0,0);
        invoke("nvnSamplerBuilderSetDefaults",SAMPLER_BUILDER,0,0,0);
        check(invoke("nvnSamplerBuilderGetDevice",SAMPLER_BUILDER,0,0,0)==DEVICE);
        context.v[0].f[0]=1.75f;invoke("nvnSamplerBuilderSetLodBias",SAMPLER_BUILDER,0,0,0);
        check(invoke("nvnSamplerInitialize",SAMPLER,SAMPLER_BUILDER,0,0)==1);
        context.v[0].f[0]=3.5f;invoke("nvnSamplerBuilderSetLodBias",SAMPLER_BUILDER,0,0,0);
        invoke("nvnSamplerGetLodBias",SAMPLER,0,0,0);check(context.v[0].f[0]==1.75f);
        invoke("nvnSamplerPoolRegisterSampler",SAMPLERS,256,SAMPLER,0);
        wr32(SAMPLER+16,0x41100000); // Mutating the source must not alias a registered descriptor.
        WebGpuSamplerParams snapshot;
        check(web_gpu_registered_sampler(SAMPLERS,256,&snapshot)&&snapshot.bias==1.75f);
        invoke("nvnSamplerPoolRegisterSamplerBuilder",SAMPLERS,256,SAMPLER_BUILDER,0);
        check(web_gpu_registered_sampler(SAMPLERS,256,&snapshot)&&snapshot.bias==3.5f);
        invoke("nvnSamplerFinalize",SAMPLER,0,0,0);
        const uint64_t PROGRAM=BASE+0x7600,SHADERS=BASE+0xb000,SHADER_CONTROL=BASE+0xb100,SHADER_DATA=BASE+0xc000;
        check(invoke("nvnProgramInitialize",PROGRAM,DEVICE,0,0)==1);
        check(invoke("nvnProgramInitialize",PROGRAM,DEVICE,0,0)==0);
        wr64(SHADERS,SHADER_DATA);wr64(SHADERS+8,SHADER_CONTROL);
        wr32(SHADER_CONTROL,0x98761234);wr32(SHADER_CONTROL+0x6f8,0x50);
        wr32(SHADER_DATA,0x12345678);wr32(SHADER_DATA+0x30,1<<10);
        check(invoke("nvnProgramSetShaders",PROGRAM,1,SHADERS,0)==1);
        check(invoke("nvnProgramSetShadersExt",PROGRAM,1,SHADERS,0)==1);
        invoke("nvnProgramFinalize",PROGRAM,0,0,0);
        check(invoke("nvnProgramInitialize",PROGRAM,DEVICE,0,0)==1);
        invoke("nvnProgramFinalize",PROGRAM,0,0,0);
        const uint64_t TEXB=BASE+0xdff8,TEXB_COPY=BASE+0xe200,TEXB_OUTPUT=BASE+0xdf00;
        invoke("nvnTextureBuilderSetDevice",TEXB,DEVICE,0,0);
        invoke("nvnTextureBuilderSetDefaults",TEXB,0,0,0);
        check(invoke("nvnTextureBuilderGetDevice",TEXB,0,0,0)==DEVICE);
        check(invoke("nvnTextureBuilderGetTarget",TEXB,0,0,0)==1);
        check(invoke("nvnTextureBuilderGetLevels",TEXB,0,0,0)==1);
        invoke("nvnTextureBuilderSetSize3D",TEXB,127,63,7);
        invoke("nvnTextureBuilderSetStorage",TEXB,POOL_OBJECT,4096,0);
        invoke("nvnTextureBuilderSetStride",TEXB,UINT64_C(0x100000004),0,0);
        uint8_t texture_builder_copy[128];web_read(TEXB,texture_builder_copy,sizeof texture_builder_copy);
        web_write(TEXB_COPY,texture_builder_copy,sizeof texture_builder_copy);
        invoke("nvnTextureBuilderSetWidth",TEXB,1,0,0);
        check(invoke("nvnTextureBuilderGetWidth",TEXB_COPY,0,0,0)==127);
        check(invoke("nvnTextureBuilderGetHeight",TEXB_COPY,0,0,0)==63);
        check(invoke("nvnTextureBuilderGetDepth",TEXB_COPY,0,0,0)==7);
        check(invoke("nvnTextureBuilderGetMemoryPool",TEXB_COPY,0,0,0)==POOL_OBJECT);
        check(invoke("nvnTextureBuilderGetMemoryOffset",TEXB_COPY,0,0,0)==4096);
        check(invoke("nvnTextureBuilderGetStride",TEXB_COPY,0,0,0)==UINT64_C(0x100000004));
        wr64(TEXB_OUTPUT,UINT64_C(0x123456789abcdef0));
        invoke("nvnTextureBuilderSetPackagedTextureLayout",TEXB_COPY,TEXB_OUTPUT,0,0);
        wr64(TEXB_OUTPUT,0);wr32(TEXB_OUTPUT+8,0xdeadbeef);
        invoke("nvnTextureBuilderGetPackagedTextureLayout",TEXB_COPY,TEXB_OUTPUT,0,0);
        check(rd64(TEXB_OUTPUT)==UINT64_C(0x123456789abcdef0)&&rd32(TEXB_OUTPUT+8)==0xdeadbeef);
        invoke("nvnTextureViewSetDefaults",STATE,0,0,0);
        check(invoke("nvnTextureViewGetLevels",STATE,TEXB_OUTPUT,TEXB_OUTPUT+4,0)==0);
        invoke("nvnTextureViewSetLevels",STATE,2,UINT64_MAX,0);
        invoke("nvnTextureViewSetLayers",STATE,1,2,0);
        WebGpuTextureView view_copy;web_read(STATE,&view_copy,sizeof view_copy);web_write(STATE_COPY,&view_copy,sizeof view_copy);
        wr32(TEXB_OUTPUT+8,0x11223344);
        check(invoke("nvnTextureViewGetLevels",STATE_COPY,TEXB_OUTPUT,TEXB_OUTPUT+4,0)==1);
        check(rd32(TEXB_OUTPUT)==2&&rd32(TEXB_OUTPUT+4)==UINT32_MAX&&rd32(TEXB_OUTPUT+8)==0x11223344);
        check(invoke("nvnTextureViewGetLayers",STATE_COPY,TEXB_OUTPUT,TEXB_OUTPUT+4,0)==1&&rd32(TEXB_OUTPUT)==1&&rd32(TEXB_OUTPUT+4)==2);
        check(invoke("nvnTextureViewCompare",STATE,STATE_COPY,0,0)==1);
        invoke("nvnTextureViewSetFormat",STATE,0x25,0,0);
        check(invoke("nvnTextureViewCompare",STATE,STATE_COPY,0,0)==0);
#ifdef WEB_GPU_WITH_SDK
        /* Self-authored texels, actual SDK sizing, native Tegra swizzler, C NVN
         * initialization and real WebGPU readback for every mip/layer/slice. */
        const int configurations[][8]={{1,16,16,1,1,0x25},{1,17,9,1,1,0x25},{4,17,9,3,5,0x25},{2,8,8,8,4,0x25},{4,16,16,2,4,0x42},
            {1,17,9,1,1,0x25,0x10,128},{1,8,4,1,1,0x22},{1,4,4,1,1,0x23},
            {4,5,3,2,2,0x31},{4,5,3,2,2,0x32},{4,5,3,2,2,0x33},{4,5,3,2,2,0x34},{4,5,3,2,2,0x35},{4,5,3,2,2,0x36}};
        uint8_t tiled[16384],linear[16384],texture_readback[16384];
        const uint64_t TEXTURE=BASE+0x7800;
        const uint64_t COPY_QB=BASE+0x7b00,COPY_Q=BASE+0x7b80,COPY_CB=BASE+0x7c00,COPY_REGION=BASE+0x7d00,COPY_VIEW=BASE+0x7d40,COPY_HANDLES=BASE+0x7d80;
        const uint64_t COPY_POOL=BASE+0x7e00,COPY_POOL_BUILDER=BASE+0x7e80,COPY_SOURCE=BASE+0x40000,COPY_CONTROL=BASE+0x38000;
        const uint64_t TEX_POOL=BASE+0x7f00,TEX_STORAGE=BASE+0x44000;
        check(gm_commit(&memory,COPY_SOURCE,16384)==GM_OK&&gm_commit(&memory,TEX_STORAGE,131072)==GM_OK);
        invoke("nvnMemoryPoolBuilderSetDefaults",COPY_POOL_BUILDER,0,0,0);
        invoke("nvnMemoryPoolBuilderSetDevice",COPY_POOL_BUILDER,DEVICE,0,0);
        invoke("nvnMemoryPoolBuilderSetStorage",COPY_POOL_BUILDER,TEX_STORAGE,131072,0);
        check(invoke("nvnMemoryPoolInitialize",TEX_POOL,COPY_POOL_BUILDER,0,0)==1);
        invoke("nvnMemoryPoolBuilderSetStorage",COPY_POOL_BUILDER,COPY_SOURCE,16384,0);
        check(invoke("nvnMemoryPoolInitialize",COPY_POOL,COPY_POOL_BUILDER,0,0)==1);
        invoke("nvnQueueBuilderSetDevice",COPY_QB,DEVICE,0,0);invoke("nvnQueueBuilderSetDefaults",COPY_QB,0,0,0);
        check(invoke("nvnQueueBuilderGetDevice",COPY_QB,0,0,0)==DEVICE);
        check(invoke("nvnQueueInitialize",COPY_Q,COPY_QB,0,0)==1);
        for(unsigned configuration=0;configuration<sizeof configurations/sizeof *configurations;configuration++) {
            const int *v=configurations[configuration];TexLayout layout;
            check(tex_layout(&layout,v[0],v[5],v[1],v[2],v[3],v[4],v[6],(size_t)v[7],-1)==0);
            if(layout.size>sizeof tiled)guest_trap("texture fixture source capacity",layout.size);
            memset(tiled,0,sizeof tiled);
            for(int layer=0;layer<layout.layers;layer++)for(int mip=0;mip<layout.levels;mip++) {
                texture_source(linear,layout.lvl_linear_size[mip],v[5],mip,layer);
                tex_swizzle_level(&layout,mip,linear,(void *)tex_level_ptr(&layout,tiled,mip,layer));
            }
            if(configuration==0){check(tiled[16]==(uint8_t)(64*73+5));check(tiled[256]==(uint8_t)(32*73+5));}
            web_write(TEX_STORAGE+65536,tiled,layout.size);
            invoke("nvnTextureBuilderSetDefaults",TEXB,0,0,0);
            invoke("nvnTextureBuilderSetDevice",TEXB,DEVICE,0,0);
            invoke("nvnTextureBuilderSetTarget",TEXB,(uint64_t)v[0],0,0);
            invoke("nvnTextureBuilderSetSize3D",TEXB,(uint64_t)v[1],(uint64_t)v[2],(uint64_t)v[3]);
            invoke("nvnTextureBuilderSetFormat",TEXB,(uint64_t)v[5],0,0);
            invoke("nvnTextureBuilderSetLevels",TEXB,(uint64_t)v[4],0,0);
            invoke("nvnTextureBuilderSetFlags",TEXB,(uint64_t)v[6],0,0);
            invoke("nvnTextureBuilderSetStride",TEXB,(uint64_t)v[7],0,0);
            invoke("nvnTextureBuilderSetStorage",TEXB,TEX_POOL,65536,0);
            last_texture_format=(unsigned)v[5];last_texture_storage=invoke("nvnTextureBuilderGetStorageSize",TEXB,0,0,0);
            check(invoke("nvnTextureInitialize",TEXTURE,TEXB,0,0)==1);
            check(invoke("nvnTextureGetWidth",TEXTURE,0,0,0)==(unsigned)v[1]);
            check(invoke("nvnTextureGetMemoryOffset",TEXTURE,0,0,0)==65536);
            uint32_t texture_handle=0;check(web_gpu_texture_handle(TEXTURE,&texture_handle)&&texture_handle);
            invoke("nvnTextureViewSetDefaults",COPY_VIEW,0,0,0);
            invoke("nvnTextureViewSetLevels",COPY_VIEW,layout.levels>1?1:0,1,0);
            if(configuration==0)invoke("nvnTextureViewSetFormat",COPY_VIEW,0x38,0,0);
            invoke("nvnTexturePoolRegisterTexture",TEXTURES,123,TEXTURE,COPY_VIEW);
            WebGpuTextureBinding registered_texture;
            check(web_gpu_registered_texture(TEXTURES,123,&registered_texture)&&registered_texture.texture==texture_handle&&registered_texture.view_handle&&!registered_texture.image);
            uint32_t original_view=registered_texture.view_handle;
            invoke("nvnTextureViewSetLevels",COPY_VIEW,99,1,0);
            check(web_gpu_registered_texture(TEXTURES,123,&registered_texture)&&registered_texture.view.lvl0==(layout.levels>1?1:0));
            if(configuration==0) {
                if(!setjmp(expected_stop)){trap_expected=1;invoke("nvnTexturePoolRegisterTexture",TEXTURES,123,TEXTURE,COPY_VIEW);trap_expected=0;check(0);}
                else {trap_expected=0;check(strstr(error,"view registration failed")!=NULL);error[0]=0;}
                check(web_gpu_registered_texture(TEXTURES,123,&registered_texture)&&registered_texture.view_handle==original_view);
            }
            invoke("nvnTextureViewSetDefaults",COPY_VIEW,0,0,0);
            invoke("nvnTexturePoolRegisterImage",TEXTURES,123,TEXTURE,COPY_VIEW);
            check(web_gpu_registered_texture(TEXTURES,123,&registered_texture)&&registered_texture.image&&registered_texture.view_handle!=original_view);
            invoke("nvnTexturePoolRegisterTexture",TEXTURES,123,0,0);
            check(!web_gpu_registered_texture(TEXTURES,123,&registered_texture));
            if(configuration==0)invoke("nvnTexturePoolRegisterTexture",TEXTURES,124,TEXTURE,0);
            for(int layer=0;layer<layout.layers;layer++)for(int mip=0;mip<layout.levels;mip++) {
                texture_source(linear,layout.lvl_linear_size[mip],v[5],mip,layer);
                size_t size=layout.lvl_linear_size[mip];
                int expanded=v[5]==0x22||v[5]==0x23;
                check(!web_gpu_request(16,texture_handle,(uint64_t)mip,(uint64_t)layer,texture_readback,expanded?size/3*4:size).status);
                if(expanded) {
                    int equal=1;uint32_t alpha=v[5]==0x22?0x3f800000:1;
                    for(size_t pixel=0;pixel<size/12;pixel++)if(memcmp(texture_readback+pixel*16,linear+pixel*12,12)||memcmp(texture_readback+pixel*16+12,&alpha,4))equal=0;
                    check(equal);
                }else if(v[5]==0x33||v[5]==0x35){
                    int equal=1;
                    for(size_t i=0;i<size;i+=4){uint32_t a,b;memcpy(&a,texture_readback+i,4);memcpy(&b,linear+i,4);
                        if(v[5]==0x35){if((a&255)!=(b&255))equal=0;a>>=8;b>>=8;}
                        uint32_t delta=a>b?a-b:b-a;if(delta>(v[5]==0x33?512u:1u))equal=0;
                    }check(equal);
                }else check(!memcmp(texture_readback,linear,size));
            }
            if(configuration==0||configuration==2||configuration==3||configuration==4) {
                uint8_t before[16384],after[16384],expected_copy[16384],source_copy[4096],second_copy[64];
                unsigned mip=layout.levels>1?1:0,w=(unsigned)layout.lvl_w[mip],h=(unsigned)layout.lvl_h[mip];
                unsigned bw=(unsigned)layout.bw,bh=(unsigned)layout.bh,bpb=(unsigned)layout.bpb;
                unsigned cw=w<4?w:4,ch=h<4?h:4,x=w>=8?4:0,y=h>=8?4:0;
                unsigned rows=(ch+bh-1)/bh,row_bytes=(cw+bw-1)/bw*bpb,row_stride=row_bytes+8,image_stride=row_stride*rows+16;
                unsigned depth=configuration==0?1:2,z=configuration==3?1:0,layer=configuration==2?1:0;
                unsigned check_layer=configuration==2?2:configuration==4?1:0;
                size_t level_size=layout.lvl_linear_size[mip];
                check(!web_gpu_request(16,texture_handle,mip,check_layer,before,level_size).status);
                invoke("nvnTextureViewSetDefaults",COPY_VIEW,0,0,0);
                invoke("nvnTextureViewSetLevels",COPY_VIEW,mip,1,0);
                if(layer)invoke("nvnTextureViewSetLayers",COPY_VIEW,layer,2,0);
                check(invoke("nvnCommandBufferInitialize",COPY_CB,DEVICE,0,0)==1);
                invoke("nvnCommandBufferAddCommandMemory",COPY_CB,POOL_OBJECT,0,81920);
                invoke("nvnCommandBufferAddControlMemory",COPY_CB,COPY_CONTROL,1024,0);
                invoke("nvnCommandBufferBeginRecording",COPY_CB,0,0,0);
                invoke("nvnCommandBufferSetCopyRowStride",COPY_CB,row_stride,0,0);
                invoke("nvnCommandBufferSetCopyImageStride",COPY_CB,image_stride,0,0);
                int32_t region[6]={(int32_t)x,(int32_t)y,(int32_t)z,(int32_t)cw,(int32_t)ch,(int32_t)depth};web_write(COPY_REGION,region,sizeof region);
                context.x[4]=COPY_REGION;context.x[5]=0;
                invoke("nvnCommandBufferCopyBufferToTexture",COPY_CB,COPY_SOURCE,TEXTURE,COPY_VIEW);
                invoke("nvnCommandBufferSetCopyRowStride",COPY_CB,0,0,0);invoke("nvnCommandBufferSetCopyImageStride",COPY_CB,0,0,0);
                unsigned sw=bw,sh=bh;
                region[0]=0;region[1]=0;region[3]=(int32_t)sw;region[4]=(int32_t)sh;region[5]=1;web_write(COPY_REGION,region,sizeof region);
                context.x[4]=COPY_REGION;context.x[5]=0;
                invoke("nvnCommandBufferCopyBufferToTexture",COPY_CB,COPY_SOURCE+8192,TEXTURE,COPY_VIEW);
                uint64_t copy_list=invoke("nvnCommandBufferEndRecording",COPY_CB,0,0,0);wr64(COPY_HANDLES,copy_list);
                invoke("nvnCommandBufferFinalize",COPY_CB,0,0,0);
                // Mutate view/region and source after recording. Only source bytes
                // must be observed at submission; command metadata is a snapshot.
                invoke("nvnTextureViewSetDefaults",COPY_VIEW,0,0,0);memset(region,0,sizeof region);web_write(COPY_REGION,region,sizeof region);
                for(unsigned i=0;i<sizeof source_copy;i++)source_copy[i]=(uint8_t)(i*29+configuration*17+31);
                for(unsigned i=0;i<sizeof second_copy;i++)second_copy[i]=(uint8_t)(i*7+201);
                web_write(COPY_SOURCE,source_copy,sizeof source_copy);web_write(COPY_SOURCE+8192,second_copy,sizeof second_copy);
                check(!web_gpu_request(16,texture_handle,mip,check_layer,after,level_size).status&&!memcmp(before,after,level_size));
                invoke("nvnQueueResetMemoryUsageCounts",COPY_Q,0,0,0);
                invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);
                check(invoke("nvnQueueGetTotalCommandMemoryUsed",COPY_Q,0,0,0)==256);
                check(invoke("nvnQueueGetTotalControlMemoryUsed",COPY_Q,0,0,0)==32);
                check(!web_gpu_request(16,texture_handle,mip,check_layer,after,level_size).status);
                memcpy(expected_copy,before,level_size);
                unsigned dest_row=(unsigned)layout.lvl_wb[mip]*bpb,dest_rows=(unsigned)layout.lvl_hb[mip];
                for(unsigned slice=0;slice<depth;slice++) {
                    if(configuration!=3&&layer+slice!=check_layer)continue;
                    unsigned dest_slice=configuration==3?z+slice:0;
                    for(unsigned row=0;row<rows;row++)memcpy(expected_copy+((dest_slice*dest_rows+y/bh+row)*dest_row+x/bw*bpb),
                        source_copy+slice*image_stride+row*row_stride,row_bytes);
                }
                if(configuration==3||check_layer==layer) {
                    unsigned dest_slice=configuration==3?z:0;
                    memcpy(expected_copy+dest_slice*dest_rows*dest_row,second_copy,bpb);
                }
                check(!memcmp(expected_copy,after,level_size));
                // Completion releases source lifetime obligations; keeping an
                // old command handle must not pin a one-shot upload pool.
                invoke("nvnQueueFinish",COPY_Q,0,0,0);
                invoke("nvnMemoryPoolFinalize",COPY_POOL,0,0,0);
                if(!setjmp(expected_stop)) {trap_expected=1;invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);trap_expected=0;check(0);}
                else {trap_expected=0;check(strstr(error,"source/texture staging failed")!=NULL);error[0]=0;}
                check(invoke("nvnMemoryPoolInitialize",COPY_POOL,COPY_POOL_BUILDER,0,0)==1);
                invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);
                check(!web_gpu_request(16,texture_handle,mip,check_layer,after,level_size).status&&!memcmp(expected_copy,after,level_size));
                invoke("nvnDeviceFinalizeCommandHandle",DEVICE,copy_list,0,0);
                if(configuration==0) {
                    if(!setjmp(expected_stop)) {trap_expected=1;invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);trap_expected=0;check(0);}
                    else {trap_expected=0;check(strstr(error,"finalised command handle")!=NULL);error[0]=0;}
                    invoke("nvnQueueSubmitCommands",COPY_Q,0,0,0);
                }
            }
            if(configuration==0) {
                /* Real NVN records -> pthread bridge -> physical target pixels.
                 * The copy must remain between the full and masked clears. */
                const uint64_t TARGET_ARRAY=COPY_HANDLES+32,CLEAR_VALUE=COPY_HANDLES+64,SCISSOR_ARRAY=COPY_HANDLES+96;
                check(invoke("nvnCommandBufferInitialize",COPY_CB,DEVICE,0,0)==1);
                invoke("nvnCommandBufferAddCommandMemory",COPY_CB,POOL_OBJECT,0,81920);
                invoke("nvnCommandBufferAddControlMemory",COPY_CB,COPY_CONTROL,1024,0);
                invoke("nvnCommandBufferBeginRecording",COPY_CB,0,0,0);
                wr64(TARGET_ARRAY,TEXTURE);wr64(TARGET_ARRAY+8,COPY_VIEW);
                invoke("nvnTextureViewSetDefaults",COPY_VIEW,0,0,0);
                context.x[4]=0;context.x[5]=0;
                invoke("nvnCommandBufferSetRenderTargets",COPY_CB,1,TARGET_ARRAY,TARGET_ARRAY+8);
                context.x[4]=16;invoke("nvnCommandBufferSetScissor",COPY_CB,0,0,16);
                wr64(CLEAR_VALUE,0);wr32(CLEAR_VALUE+8,0);wr32(CLEAR_VALUE+12,0x3f800000);
                invoke("nvnCommandBufferClearColor",COPY_CB,0,CLEAR_VALUE,15);
                int32_t clear_region[6]={0,0,0,4,4,1};web_write(COPY_REGION,clear_region,sizeof clear_region);
                uint8_t clear_source[64];for(unsigned i=0;i<64;i++)clear_source[i]=(uint8_t[]){64,128,192,255}[i%4];web_write(COPY_SOURCE,clear_source,64);
                context.x[4]=COPY_REGION;context.x[5]=0;
                invoke("nvnCommandBufferCopyBufferToTexture",COPY_CB,COPY_SOURCE,TEXTURE,0);
                context.x[4]=2;invoke("nvnCommandBufferSetScissor",COPY_CB,1,1,2);
                wr32(CLEAR_VALUE,0x3f800000);wr32(CLEAR_VALUE+12,0);
                invoke("nvnCommandBufferClearColor",COPY_CB,0,CLEAR_VALUE,1);
                uint64_t render_list=invoke("nvnCommandBufferEndRecording",COPY_CB,0,0,0);
                wr64(COPY_HANDLES,render_list);wr64(TARGET_ARRAY,0);
                invoke("nvnTextureViewSetLevels",COPY_VIEW,9,1,0);
                for(unsigned i=0;i<4;i++)wr32(CLEAR_VALUE+i*4,0x7fc00000);
                if(!setjmp(expected_stop)){trap_expected=1;invoke("nvnTextureFinalize",TEXTURE,0,0,0);trap_expected=0;check(0);}
                else{trap_expected=0;check(strstr(error,"referenced by an active submission")!=NULL);error[0]=0;}
                invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);
                invoke("nvnDeviceFinalizeCommandHandle",DEVICE,render_list,0,0);
                check(!web_gpu_request(16,texture_handle,0,0,texture_readback,16*16*4).status);
                for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++) {
                    uint8_t expected_pixel[4]={0,0,0,255};
                    if(x<4&&y<4)memcpy(expected_pixel,(uint8_t[]){64,128,192,255},4);
                    if(x>=1&&x<3&&y>=1&&y<3)expected_pixel[0]=255;
                    check(!memcmp(texture_readback+(y*16+x)*4,expected_pixel,4));
                }
                if(!setjmp(expected_stop)){trap_expected=1;invoke("nvnTextureFinalize",TEXTURE,0,0,0);trap_expected=0;check(0);}
                else{trap_expected=0;check(strstr(error,"referenced by an active submission")!=NULL);error[0]=0;}
                invoke("nvnCommandBufferBeginRecording",COPY_CB,0,0,0);
                wr64(CLEAR_VALUE,UINT64_C(0x3f80000000000000));wr64(CLEAR_VALUE+8,0);
                invoke("nvnCommandBufferClearColor",COPY_CB,0,CLEAR_VALUE,2);
                render_list=invoke("nvnCommandBufferEndRecording",COPY_CB,0,0,0);wr64(COPY_HANDLES,render_list);
                invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);
                invoke("nvnDeviceFinalizeCommandHandle",DEVICE,render_list,0,0);
                check(!web_gpu_request(16,texture_handle,0,0,texture_readback,16*16*4).status);
                for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++)if(x>=1&&x<3&&y>=1&&y<3)
                    check(!memcmp(texture_readback+(y*16+x)*4,(uint8_t[]){255,255,192,255},4));
                invoke("nvnCommandBufferBeginRecording",COPY_CB,0,0,0);
                int32_t clipped_scissor[4]={-2,-1,3,2};web_write(SCISSOR_ARRAY,clipped_scissor,16);
                invoke("nvnCommandBufferSetScissors",COPY_CB,0,1,SCISSOR_ARRAY);
                wr64(CLEAR_VALUE,UINT64_C(0x3f8000003f800000));wr32(CLEAR_VALUE+8,0);wr32(CLEAR_VALUE+12,0x3f800000);
                invoke("nvnCommandBufferClearColor",COPY_CB,0,CLEAR_VALUE,15);
                context.x[4]=0;context.x[5]=0;invoke("nvnCommandBufferSetRenderTargets",COPY_CB,0,0,0);
                render_list=invoke("nvnCommandBufferEndRecording",COPY_CB,0,0,0);wr64(COPY_HANDLES,render_list);
                for(unsigned i=0;i<4;i++)wr32(SCISSOR_ARRAY+i*4,0);
                invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);
                invoke("nvnDeviceFinalizeCommandHandle",DEVICE,render_list,0,0);
                check(!web_gpu_request(16,texture_handle,0,0,texture_readback,16*16*4).status);
                check(!memcmp(texture_readback,(uint8_t[]){255,255,0,255},4));
                check(!memcmp(texture_readback+4,(uint8_t[]){64,128,192,255},4));
                check(!memcmp(texture_readback+16*4,(uint8_t[]){64,128,192,255},4));
                invoke("nvnCommandBufferBeginRecording",COPY_CB,0,0,0);
                wr64(TARGET_ARRAY,TEXTURE);wr64(TARGET_ARRAY+8,COPY_VIEW);
                invoke("nvnTextureViewSetDefaults",COPY_VIEW,0,0,0);invoke("nvnTextureViewSetFormat",COPY_VIEW,0x38,0,0);
                context.x[4]=0;context.x[5]=0;invoke("nvnCommandBufferSetRenderTargets",COPY_CB,1,TARGET_ARRAY,TARGET_ARRAY+8);
                context.x[4]=16;invoke("nvnCommandBufferSetScissor",COPY_CB,0,0,16);
                wr32(CLEAR_VALUE,0x3f000000);wr32(CLEAR_VALUE+4,0x3e800000);wr32(CLEAR_VALUE+8,0x3f400000);wr32(CLEAR_VALUE+12,0x3f800000);
                invoke("nvnCommandBufferClearColor",COPY_CB,0,CLEAR_VALUE,15);
                context.x[4]=0;context.x[5]=0;invoke("nvnCommandBufferSetRenderTargets",COPY_CB,0,0,0);
                render_list=invoke("nvnCommandBufferEndRecording",COPY_CB,0,0,0);wr64(COPY_HANDLES,render_list);
                invoke("nvnTextureViewSetFormat",COPY_VIEW,0x25,0,0);
                invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);
                invoke("nvnDeviceFinalizeCommandHandle",DEVICE,render_list,0,0);
                check(!web_gpu_request(16,texture_handle,0,0,texture_readback,16*16*4).status);
                for(unsigned pixel=0;pixel<256;pixel++) {
                    int equal=1;for(unsigned channel=0;channel<4;channel++) {
                        int difference=(int)texture_readback[pixel*4+channel]-(int[]){188,137,225,255}[channel];
                        if(difference < -1||difference>1)equal=0;
                    }check(equal);
                }
                invoke("nvnCommandBufferFinalize",COPY_CB,0,0,0);
                if(graphics_fixture())for(unsigned textured=0;textured<2;textured++) {
                    const uint64_t DRAW_PROGRAM=BASE+0x7700,DRAW_SOURCE=BASE+0x7980,VERTICES=STORAGE+65536,UNIFORM_DATA=STORAGE+65604;
                    uint64_t sampled=0;
                    if(textured){
                        check(invoke("nvnTextureInitialize",DRAW_SOURCE,TEXB,0,0)==1);
                        invoke("nvnTexturePoolRegisterTexture",TEXTURES,125,DRAW_SOURCE,0);
                        invoke("nvnSamplerBuilderSetDevice",SAMPLER_BUILDER,DEVICE,0,0);invoke("nvnSamplerBuilderSetDefaults",SAMPLER_BUILDER,0,0,0);
                        invoke("nvnSamplerBuilderSetMinMagFilter",SAMPLER_BUILDER,0,0,0);
                        invoke("nvnSamplerPoolRegisterSamplerBuilder",SAMPLERS,257,SAMPLER_BUILDER,0);
                        sampled=invoke("nvnDeviceGetTextureHandle",DEVICE,125,257,0);
                    }
                    check(invoke("nvnProgramInitialize",DRAW_PROGRAM,DEVICE,0,0)==1);
                    wr64(SHADERS,SHADER_DATA);wr64(SHADERS+8,SHADER_CONTROL);wr64(SHADERS+16,SHADER_DATA+128);wr64(SHADERS+24,SHADER_CONTROL);
                    wr32(SHADER_DATA+128,0x12345678);wr32(SHADER_DATA+128+0x30,5<<10);
                    wr32(SHADER_CONTROL+0x704,textured?0x54455854:0);
                    check(invoke("nvnProgramSetShaders",DRAW_PROGRAM,2,SHADERS,0)==1);
                    wr32(STATE_COPY,0x2e);wr32(STATE_COPY+16,16);wr32(STATE_COPY+20,0);
                    web_write(STATE_COPY+32,(uint8_t[]){0,3,8,0},4);
                    check(invoke("nvnCommandBufferInitialize",COPY_CB,DEVICE,0,0)==1);
                    invoke("nvnCommandBufferAddCommandMemory",COPY_CB,POOL_OBJECT,0,81920);
                    invoke("nvnCommandBufferAddControlMemory",COPY_CB,COPY_CONTROL,1024,0);
                    invoke("nvnCommandBufferBeginRecording",COPY_CB,0,0,0);
                    invoke("nvnTextureViewSetDefaults",COPY_VIEW,0,0,0);wr64(TARGET_ARRAY,TEXTURE);
                    context.x[4]=0;context.x[5]=0;invoke("nvnCommandBufferSetRenderTargets",COPY_CB,1,TARGET_ARRAY,0);
                    context.x[4]=16;invoke("nvnCommandBufferSetScissor",COPY_CB,0,0,16);
                    context.x[4]=16;invoke("nvnCommandBufferSetViewport",COPY_CB,0,0,16);
                    invoke("nvnCommandBufferBindVertexAttribState",COPY_CB,1,STATE_COPY,0);
                    invoke("nvnCommandBufferBindVertexStreamState",COPY_CB,1,STATE_COPY+16,0);
                    invoke("nvnCommandBufferBindColorState",COPY_CB,STATE_COPY+32,0,0);
                    invoke("nvnCommandBufferBindVertexBuffer",COPY_CB,0,VERTICES,48);
                    context.x[4]=16;invoke("nvnCommandBufferBindUniformBuffer",COPY_CB,1,0,UNIFORM_DATA);
                    invoke("nvnCommandBufferBindProgram",COPY_CB,DRAW_PROGRAM,3,0);
                    if(textured){invoke("nvnCommandBufferSetTexturePool",COPY_CB,TEXTURES,0,0);invoke("nvnCommandBufferSetSamplerPool",COPY_CB,SAMPLERS,0,0);
                        invoke("nvnCommandBufferBindTexture",COPY_CB,1,0,sampled);}
                    invoke("nvnCommandBufferDrawArrays",COPY_CB,4,0,3);
                    /* Unbind releases queue-held pools/programs after replay. */
                    invoke("nvnCommandBufferBindVertexBuffer",COPY_CB,0,0,0);
                    if(textured)invoke("nvnCommandBufferBindTexture",COPY_CB,1,0,0);
                    context.x[4]=0;invoke("nvnCommandBufferBindUniformBuffer",COPY_CB,1,0,0);
                    invoke("nvnCommandBufferBindProgram",COPY_CB,0,3,0);
                    context.x[4]=0;context.x[5]=0;invoke("nvnCommandBufferSetRenderTargets",COPY_CB,0,0,0);
                    uint64_t draw_list=invoke("nvnCommandBufferEndRecording",COPY_CB,0,0,0);wr64(COPY_HANDLES,draw_list);
                    invoke("nvnCommandBufferFinalize",COPY_CB,0,0,0);invoke("nvnProgramFinalize",DRAW_PROGRAM,0,0,0);
                    /* Values are immutable; buffer contents are consumed at submit. */
                    web_write(STATE_COPY,(uint8_t[64]){0},64);
                    web_write(VERTICES,(float[]){-1,-1,0.25f,1,3,-1,0.25f,1,-1,3,0.25f,1},48);
                    if(textured)web_write(UNIFORM_DATA,(float[]){1,1,1,1},16);else web_write(UNIFORM_DATA,(float[]){0.2f,0.4f,0.6f,1},16);
                    invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);
                    invoke("nvnDeviceFinalizeCommandHandle",DEVICE,draw_list,0,0);
                    check(!web_gpu_request(16,texture_handle,0,0,texture_readback,16*16*4).status);
                    if(textured)texture_source(linear,16*16*4,0x25,0,0);
                    for(unsigned pixel=0;pixel<256;pixel++)for(unsigned channel=0;channel<4;channel++){
                        int delta=(int)texture_readback[pixel*4+channel]-(textured?(int)linear[pixel*4+channel]:(int[]){51,102,153,255}[channel]);check(delta>=-1&&delta<=1);
                    }
                    if(textured){invoke("nvnTexturePoolRegisterTexture",TEXTURES,125,0,0);invoke("nvnTextureFinalize",DRAW_SOURCE,0,0,0);}
                }
            }
            if(configuration==0&&graphics_fixture()) {
                /* Two real recorded dispatches: the second stages the first
                 * output from guest memory. This detects missing GPU->guest
                 * writeback and a later upload erasing computed bytes. */
                const uint64_t CP=BASE+0x7780,CC=BASE+0xd000,DATA=BASE+0xd800;
                const uint64_t PARAMS=STORAGE+65700,INPUT=STORAGE+65804,OUTPUT=STORAGE+65932,NEXT=STORAGE+66060;
                wr32(CC,0x98761234);wr32(CC+0x6f8,32);wr32(CC+0x714,5);wr32(CC+0x748,4);wr32(CC+0x74c,1);wr32(CC+0x750,1);
                wr64(SHADERS,DATA);wr64(SHADERS+8,CC);check(invoke("nvnProgramInitialize",CP,DEVICE,0,0)==1);
                check(invoke("nvnProgramSetShaders",CP,1,SHADERS,0)==1);
                check(invoke("nvnCommandBufferInitialize",COPY_CB,DEVICE,0,0)==1);
                invoke("nvnCommandBufferAddCommandMemory",COPY_CB,POOL_OBJECT,0,81920);invoke("nvnCommandBufferAddControlMemory",COPY_CB,COPY_CONTROL,1024,0);
                invoke("nvnCommandBufferBeginRecording",COPY_CB,0,0,0);invoke("nvnCommandBufferBindProgram",COPY_CB,CP,32,0);
                context.x[4]=16;invoke("nvnCommandBufferBindUniformBuffer",COPY_CB,5,0,PARAMS);
                context.x[4]=32;invoke("nvnCommandBufferBindStorageBuffer",COPY_CB,5,0,INPUT);
                context.x[4]=32;invoke("nvnCommandBufferBindStorageBuffer",COPY_CB,5,1,OUTPUT);
                invoke("nvnCommandBufferDispatchCompute",COPY_CB,0,1,1);invoke("nvnCommandBufferDispatchCompute",COPY_CB,2,1,1);
                context.x[4]=32;invoke("nvnCommandBufferBindStorageBuffer",COPY_CB,5,0,OUTPUT);
                context.x[4]=32;invoke("nvnCommandBufferBindStorageBuffer",COPY_CB,5,1,NEXT);
                invoke("nvnCommandBufferDispatchCompute",COPY_CB,2,1,1);
                for(unsigned i=0;i<2;i++){context.x[4]=0;invoke("nvnCommandBufferBindStorageBuffer",COPY_CB,5,i,0);}
                context.x[4]=0;invoke("nvnCommandBufferBindUniformBuffer",COPY_CB,5,0,0);invoke("nvnCommandBufferBindProgram",COPY_CB,0,32,0);
                uint64_t compute_list=invoke("nvnCommandBufferEndRecording",COPY_CB,0,0,0);wr64(COPY_HANDLES,compute_list);
                invoke("nvnCommandBufferFinalize",COPY_CB,0,0,0);invoke("nvnProgramFinalize",CP,0,0,0);
                wr32(PARAMS,7);wr32(PARAMS+4,5);wr32(PARAMS+8,0);wr32(PARAMS+12,0);
                for(unsigned i=0;i<8;i++){wr32(INPUT+i*4,i+1);wr32(OUTPUT+i*4,0xdeadbeef);wr32(NEXT+i*4,0xdeadbeef);}
                invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);invoke("nvnDeviceFinalizeCommandHandle",DEVICE,compute_list,0,0);
                for(unsigned i=0;i<7;i++){check(rd32(INPUT+i*4)==i+1);check(rd32(OUTPUT+i*4)==(i+1)*3+5);check(rd32(NEXT+i*4)==((i+1)*3+5)*3+5);}
                check(rd32(OUTPUT+28)==0xdeadbeef&&rd32(NEXT+28)==0xdeadbeef);
                wr64(SHADERS,SHADER_DATA);wr64(SHADERS+8,SHADER_CONTROL);
            }
            if(configuration==6||configuration==7) {
                const uint64_t TARGET_ARRAY=COPY_HANDLES+32,CLEAR_VALUE=COPY_HANDLES+64;
                unsigned integer=configuration==7;uint32_t color[4];
                if(integer)memcpy(color,(uint32_t[]){UINT32_MAX,123,100,9},16);
                else memcpy(color,(float[]){-4,0.5f,8,999},16);
                web_write(CLEAR_VALUE,color,16);wr64(TARGET_ARRAY,TEXTURE);
                check(invoke("nvnCommandBufferInitialize",COPY_CB,DEVICE,0,0)==1);
                invoke("nvnCommandBufferAddCommandMemory",COPY_CB,POOL_OBJECT,0,81920);
                invoke("nvnCommandBufferAddControlMemory",COPY_CB,COPY_CONTROL,1024,0);
                invoke("nvnCommandBufferBeginRecording",COPY_CB,0,0,0);
                context.x[4]=0;context.x[5]=0;invoke("nvnCommandBufferSetRenderTargets",COPY_CB,1,TARGET_ARRAY,0);
                context.x[4]=(unsigned)v[2];invoke("nvnCommandBufferSetScissor",COPY_CB,0,0,(unsigned)v[1]);
                invoke(integer?"nvnCommandBufferClearColorui":"nvnCommandBufferClearColor",COPY_CB,0,CLEAR_VALUE,15);
                uint64_t render_list=invoke("nvnCommandBufferEndRecording",COPY_CB,0,0,0);wr64(COPY_HANDLES,render_list);
                invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);
                invoke("nvnDeviceFinalizeCommandHandle",DEVICE,render_list,0,0);
                check(!web_gpu_request(16,texture_handle,0,0,texture_readback,(size_t)v[1]*v[2]*16).status);
                for(unsigned i=0;i<(unsigned)(v[1]*v[2]*4);i++) {
                    uint32_t actual;memcpy(&actual,texture_readback+i*4,4);
                    check(actual==(i%4==3?(integer?1:0x3f800000):color[i%4]));
                }
                invoke("nvnCommandBufferBeginRecording",COPY_CB,0,0,0);
                context.x[4]=0;context.x[5]=0;invoke("nvnCommandBufferSetRenderTargets",COPY_CB,0,0,0);
                render_list=invoke("nvnCommandBufferEndRecording",COPY_CB,0,0,0);wr64(COPY_HANDLES,render_list);
                invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);
                invoke("nvnDeviceFinalizeCommandHandle",DEVICE,render_list,0,0);
                invoke("nvnCommandBufferFinalize",COPY_CB,0,0,0);
            }
            if(v[5]>=0x31&&v[5]<=0x36) {
                /* Immutable depth-view, scissor, value and write-mask snapshots
                 * must reach the selected mip/layer through the C recorder. */
                check(invoke("nvnCommandBufferInitialize",COPY_CB,DEVICE,0,0)==1);
                invoke("nvnCommandBufferAddCommandMemory",COPY_CB,POOL_OBJECT,0,81920);
                invoke("nvnCommandBufferAddControlMemory",COPY_CB,COPY_CONTROL,1024,0);
                invoke("nvnCommandBufferBeginRecording",COPY_CB,0,0,0);
                invoke("nvnTextureViewSetDefaults",COPY_VIEW,0,0,0);
                invoke("nvnTextureViewSetLevels",COPY_VIEW,1,1,0);invoke("nvnTextureViewSetLayers",COPY_VIEW,1,1,0);
                context.x[4]=TEXTURE;context.x[5]=COPY_VIEW;
                invoke("nvnCommandBufferSetRenderTargets",COPY_CB,0,0,0);
                context.x[4]=1;invoke("nvnCommandBufferSetScissor",COPY_CB,0,0,2);
                context.v[0].f[0]=0.5f;invoke("nvnCommandBufferClearDepthStencil",COPY_CB,1,0xa5,255);
                context.x[4]=1;invoke("nvnCommandBufferSetScissor",COPY_CB,UINT64_MAX,0,2);
                context.v[0].f[0]=0.75f;invoke("nvnCommandBufferClearDepthStencil",COPY_CB,1,0x3c,15);
                context.x[4]=0;context.x[5]=0;invoke("nvnCommandBufferSetRenderTargets",COPY_CB,0,0,0);
                uint64_t depth_list=invoke("nvnCommandBufferEndRecording",COPY_CB,0,0,0);wr64(COPY_HANDLES,depth_list);
                context.v[0].w[0]=0x7fc00000;invoke("nvnTextureViewSetLevels",COPY_VIEW,99,1,0);
                invoke("nvnQueueSubmitCommands",COPY_Q,1,COPY_HANDLES,0);
                invoke("nvnDeviceFinalizeCommandHandle",DEVICE,depth_list,0,0);invoke("nvnCommandBufferFinalize",COPY_CB,0,0,0);
                size_t bpp=v[5]==0x31?1:v[5]==0x32?2:v[5]==0x36?8:4;
                check(!web_gpu_request(16,texture_handle,1,1,texture_readback,2*bpp).status);
                for(unsigned pixel=0;pixel<2;pixel++) {
                    const uint8_t *p=texture_readback+pixel*bpp;uint32_t word=0;memcpy(&word,p,bpp<4?bpp:4);
                    unsigned stencil_expected=pixel?0xa5:0xac;float depth_expected=pixel?0.5f:0.75f;
                    if(v[5]==0x31)check(*p==stencil_expected);
                    else if(v[5]==0x32){uint32_t expected_depth=(uint32_t)(depth_expected*65535.0f+0.5f);check(word+1>=expected_depth&&word<=expected_depth+1);}
                    else if(v[5]==0x33){uint64_t expected_depth=(uint64_t)((double)depth_expected*4294967295.0);check((uint64_t)word+512>=expected_depth&&(uint64_t)word<=expected_depth+512);}
                    else if(v[5]==0x35){uint32_t depth=word>>8,expected_depth=(uint32_t)(depth_expected*16777215.0f);check(depth+1>=expected_depth&&depth<=expected_depth+1&&(word&255)==stencil_expected);}
                    else{float depth;memcpy(&depth,p,4);check(depth==depth_expected);if(v[5]==0x36)check(p[4]==stencil_expected);}
                }
            }
            invoke("nvnTextureFinalize",TEXTURE,0,0,0);
            check(!web_gpu_texture_handle(TEXTURE,&texture_handle));
            check(web_gpu_request(16,texture_handle,0,0,texture_readback,4).status!=0);
            if(configuration==0) {
                invoke("nvnTextureBuilderSetFlags",TEXB,0x10,0,0);
                invoke("nvnTextureBuilderSetStride",TEXB,1,0,0);
                check(invoke("nvnTextureInitialize",TEXTURE,TEXB,0,0)==0);
            }
            if(configuration==8) {
                invoke("nvnTextureBuilderSetStorage",TEXB,TEX_POOL,65540,0);
                check(invoke("nvnTextureInitialize",TEXTURE,TEXB,0,0)==0);
            }
        }
        invoke("nvnQueueFinalize",COPY_Q,0,0,0);invoke("nvnMemoryPoolFinalize",COPY_POOL,0,0,0);invoke("nvnMemoryPoolFinalize",TEX_POOL,0,0,0);
#endif
        invoke("nvnQueueBuilderSetDefaults",QUEUE_BUILDER,0,0,0);
        invoke("nvnQueueBuilderSetDevice",QUEUE_BUILDER,DEVICE,0,0);
        check(invoke("nvnQueueBuilderGetDevice",QUEUE_BUILDER,0,0,0)==DEVICE);
        check(invoke("nvnQueueBuilderGetQueueMemorySize",QUEUE_BUILDER,0,0,0)==344064);
        check(invoke("nvnQueueBuilderGetMemorySize",QUEUE_BUILDER,0,0,0)==0);
        check(invoke("nvnQueueInitialize",QUEUE,QUEUE_BUILDER,0,0)==1);
        check(invoke("nvnSyncInitialize",SYNC,DEVICE,0,0)==1);
        check(invoke("nvnSyncWait",SYNC,0,0,0)==3);
        check(invoke("nvnQueueWaitSync",QUEUE,SYNC,0,0)==0);
        invoke("nvnQueueFlush",QUEUE,0,0,0);
        invoke("nvnQueueFinish",QUEUE,0,0,0);
        check(invoke("nvnQueueGetError",QUEUE,0,0,0)==0);
        check(invoke("nvnQueueGetTotalCommandMemoryUsed",QUEUE,0,0,0)==0);
        check(invoke("nvnCommandBufferInitialize",COMMANDS,DEVICE,0,0)==1);
        check(invoke("nvnCommandBufferIsRecording",COMMANDS,0,0,0)==0);
        invoke("nvnCommandBufferSetMemoryCallback",COMMANDS,0x7100123456ULL,0,0);
        invoke("nvnCommandBufferSetMemoryCallbackData",COMMANDS,44,0,0);
        check(invoke("nvnCommandBufferGetMemoryCallback",COMMANDS,0,0,0)==0x7100123456ULL);
        check(invoke("nvnCommandBufferGetMemoryCallbackData",COMMANDS,0,0,0)==44);
        invoke("nvnCommandBufferAddCommandMemory",COMMANDS,POOL_OBJECT,0,81920);
        invoke("nvnCommandBufferAddControlMemory",COMMANDS,CONTROL,1024,0);
        check(invoke("nvnCommandBufferGetCommandMemorySize",COMMANDS,0,0,0)==81920);
        check(invoke("nvnCommandBufferGetControlMemorySize",COMMANDS,0,0,0)==1024);
        invoke("nvnCommandBufferBeginRecording",COMMANDS,0,0,0);
        check(invoke("nvnCommandBufferIsRecording",COMMANDS,0,0,0)==1);
        invoke("nvnCommandBufferSetTexturePool",COMMANDS,TEXTURES,0,0);
        invoke("nvnCommandBufferSetSamplerPool",COMMANDS,SAMPLERS,0,0);
        invoke("nvnCommandBufferSetTexturePool",COMMANDS,TEXTURES,0,0);
        if(!setjmp(expected_stop)) {
            trap_expected=1;invoke("nvnCommandBufferDrawArrays",COMMANDS,14,0,3);
            trap_expected=0;check(0);
        } else {trap_expected=0;check(strstr(error,"NVN draw topology/index type needs lowering")!=NULL);error[0]=0;}
        uint64_t first_list=invoke("nvnCommandBufferEndRecording",COMMANDS,0,0,0);
        check(first_list!=0&&invoke("nvnCommandBufferIsRecording",COMMANDS,0,0,0)==0);
        int sampler=-1;uint64_t bound_pool=0;
        check(web_gpu_command_pool_binding(first_list,0,&sampler,&bound_pool)&&!sampler&&bound_pool==TEXTURES);
        check(web_gpu_command_pool_binding(first_list,1,&sampler,&bound_pool)&&sampler&&bound_pool==SAMPLERS);
        check(web_gpu_command_pool_binding(first_list,2,&sampler,&bound_pool)&&!sampler&&bound_pool==TEXTURES);
        check(!web_gpu_command_pool_binding(first_list,3,&sampler,&bound_pool));
        check(invoke("nvnCommandBufferGetCommandMemoryUsed",COMMANDS,0,0,0)==48);
        if(!setjmp(expected_stop)) {
            trap_expected=1;invoke("nvnTexturePoolFinalize",TEXTURES,0,0,0);trap_expected=0;check(0);
        } else {trap_expected=0;check(strstr(error,"referenced by recorded commands")!=NULL);error[0]=0;}
        check(invoke("nvnCommandBufferGetControlMemoryUsed",COMMANDS,0,0,0)==32);
        check(invoke("nvnCommandBufferGetControlMemoryFree",COMMANDS,0,0,0)==992);
        invoke("nvnCommandBufferBeginRecording",COMMANDS,0,0,0);
        uint64_t second_list=invoke("nvnCommandBufferEndRecording",COMMANDS,0,0,0);
        check(second_list!=first_list&&second_list!=0);
        check(!web_gpu_command_pool_binding(second_list,0,&sampler,&bound_pool));
        check(web_gpu_command_pool_binding(first_list,1,&sampler,&bound_pool)&&sampler&&bound_pool==SAMPLERS);
        invoke("nvnDeviceFinalizeCommandHandle",DEVICE,first_list,0,0);
        check(!web_gpu_command_pool_binding(first_list,0,&sampler,&bound_pool));
        invoke("nvnDeviceFinalizeCommandHandle",DEVICE,second_list,0,0);
        control_reuse_checks();
        /* Real refill path: host recording -> protected guest callback ->
         * reentrant NVN allocation, then replay the original pending command. */
        check(invoke("nvnCommandBufferInitialize",REFILL_COMMANDS,DEVICE,0,0)==1);
        invoke("nvnCommandBufferSetMemoryCallback",REFILL_COMMANDS,REFILL_CALLBACK,0,0);
        invoke("nvnCommandBufferSetMemoryCallbackData",REFILL_COMMANDS,44,0,0);
        invoke("nvnCommandBufferAddCommandMemory",REFILL_COMMANDS,POOL_OBJECT,0,16);
        invoke("nvnCommandBufferAddControlMemory",REFILL_COMMANDS,CONTROL+2048,32,0);
        invoke("nvnCommandBufferBeginRecording",REFILL_COMMANDS,0,0,0);
        invoke("nvnCommandBufferSetTexturePool",REFILL_COMMANDS,TEXTURES,0,0);check(callback_calls==0);
        context.x[8]=0x12345678;context.v[0].d[0]=0x123456789abcdef0ULL;
        invoke("nvnCommandBufferSetSamplerPool",REFILL_COMMANDS,SAMPLERS,0,0);
        check(callback_calls==1&&callback_last_event==0&&callback_last_minimum==256);
        check(context.x[8]==0x12345678&&context.v[0].d[0]==0x123456789abcdef0ULL);
        context.x[4]=0;invoke("nvnCommandBufferSetRenderTargets",REFILL_COMMANDS,0,0,0);
        check(callback_calls==2&&callback_last_minimum==512);
        uint64_t refill_list1=invoke("nvnCommandBufferEndRecording",REFILL_COMMANDS,0,0,0);
        check(web_gpu_command_pool_binding(refill_list1,1,&sampler,&bound_pool)&&sampler&&bound_pool==SAMPLERS);
        check(invoke("nvnCommandBufferGetCommandMemoryUsed",REFILL_COMMANDS,0,0,0)==320);
        check(invoke("nvnCommandBufferGetCommandMemoryFree",REFILL_COMMANDS,0,0,0)==192);
        invoke("nvnCommandBufferSetCommandMemoryCallbackEnabled",REFILL_COMMANDS,0,0,0);
        invoke("nvnCommandBufferBeginRecording",REFILL_COMMANDS,0,0,0);
        uint64_t refill_list2=invoke("nvnCommandBufferEndRecording",REFILL_COMMANDS,0,0,0);
        check(callback_calls==3&&callback_last_event==1&&callback_last_minimum==256);
        check(invoke("nvnCommandBufferGetControlMemoryUsed",REFILL_COMMANDS,0,0,0)==32);
        invoke("nvnCommandBufferAddCommandMemory",REFILL_COMMANDS,POOL_OBJECT,0,0);
        invoke("nvnCommandBufferBeginRecording",REFILL_COMMANDS,0,0,0);
        if(!setjmp(expected_stop)){trap_expected=1;invoke("nvnCommandBufferSetTexturePool",REFILL_COMMANDS,TEXTURES,0,0);trap_expected=0;check(0);}
        else{trap_expected=0;check(strstr(error,"command memory exhausted")!=NULL&&callback_calls==3);error[0]=0;}
        invoke("nvnCommandBufferSetCommandMemoryCallbackEnabled",REFILL_COMMANDS,1,0,0);
        for(callback_mode=1;callback_mode<=3;callback_mode++){
            callback_fault[0]=0;
            if(!setjmp(expected_stop)){trap_expected=1;invoke("nvnCommandBufferSetTexturePool",REFILL_COMMANDS,TEXTURES,0,0);trap_expected=0;check(0);}
            else{trap_expected=0;check(strstr(error,callback_mode==1?"insufficient command memory":"callback failed")!=NULL);error[0]=0;}
            if(callback_mode==2)check(strstr(callback_fault,"recursive NVN memory refill")!=NULL);
            if(callback_mode==3)check(strstr(callback_fault,"finalising command buffer during refill")!=NULL);
        }
        callback_mode=0;check(callback_calls==6);
        invoke("nvnCommandBufferAddCommandMemory",REFILL_COMMANDS,POOL_OBJECT,0,16);
        invoke("nvnCommandBufferSetTexturePool",REFILL_COMMANDS,TEXTURES,0,0);
        uint64_t refill_list3=invoke("nvnCommandBufferEndRecording",REFILL_COMMANDS,0,0,0);
        invoke("nvnDeviceFinalizeCommandHandle",DEVICE,refill_list1,0,0);
        invoke("nvnDeviceFinalizeCommandHandle",DEVICE,refill_list2,0,0);
        invoke("nvnDeviceFinalizeCommandHandle",DEVICE,refill_list3,0,0);
        invoke("nvnCommandBufferFinalize",REFILL_COMMANDS,0,0,0);
        /* Record two versions, then destroy the source object before submission.
         * Queue stage state must outlive both command handles and that object. */
        check(invoke("nvnProgramInitialize",PROGRAM,DEVICE,0,0)==1);
        check(invoke("nvnProgramSetShaders",PROGRAM,1,SHADERS,0)==1);
        if(!setjmp(expected_stop)) {
            trap_expected=1;invoke("nvnCommandBufferBindProgram",COMMANDS,PROGRAM,1,0);trap_expected=0;check(0);
        } else {trap_expected=0;check(strstr(error,"outside command recording")!=NULL);error[0]=0;}
        invoke("nvnCommandBufferBeginRecording",COMMANDS,0,0,0);
        if(!setjmp(expected_stop)) {
            trap_expected=1;invoke("nvnCommandBufferBindProgram",COMMANDS,PROGRAM,64,0);trap_expected=0;check(0);
        } else {trap_expected=0;check(strstr(error,"invalid NVN shader stage mask")!=NULL);error[0]=0;}
        invoke("nvnCommandBufferBindProgram",COMMANDS,PROGRAM,3,0);
        uint64_t program_list=invoke("nvnCommandBufferEndRecording",COMMANDS,0,0,0);
        uint32_t stages=0,old_snapshot=0,new_snapshot=0,unbound=99;
        check(web_gpu_command_program_binding(program_list,0,&stages,&old_snapshot)&&stages==3&&old_snapshot);
        wr32(SHADER_DATA+0x34,0x12345678);
        check(invoke("nvnProgramSetShaders",PROGRAM,1,SHADERS,0)==1);
        invoke("nvnCommandBufferBeginRecording",COMMANDS,0,0,0);
        invoke("nvnCommandBufferBindProgram",COMMANDS,PROGRAM,1,0);
        uint64_t changed_list=invoke("nvnCommandBufferEndRecording",COMMANDS,0,0,0);
        check(web_gpu_command_program_binding(changed_list,0,&stages,&new_snapshot)&&stages==1&&new_snapshot&&new_snapshot!=old_snapshot);
        invoke("nvnProgramFinalize",PROGRAM,0,0,0);
        invoke("nvnCommandBufferBeginRecording",COMMANDS,0,0,0);
        if(!setjmp(expected_stop)) {
            trap_expected=1;invoke("nvnCommandBufferBindProgram",COMMANDS,PROGRAM,1,0);trap_expected=0;check(0);
        } else {trap_expected=0;check(strstr(error,"invalid NVN program for command binding")!=NULL);error[0]=0;}
        uint64_t rejected_list=invoke("nvnCommandBufferEndRecording",COMMANDS,0,0,0);
        check(!web_gpu_command_program_binding(rejected_list,0,&stages,&unbound));
        invoke("nvnDeviceFinalizeCommandHandle",DEVICE,rejected_list,0,0);
        wr64(CONTROL+512,program_list);wr64(CONTROL+520,changed_list);
        invoke("nvnQueueSubmitCommands",QUEUE,2,CONTROL+512,0);
        check(web_gpu_queue_program_snapshot(QUEUE,0)==new_snapshot);
        check(web_gpu_queue_program_snapshot(QUEUE,1)==old_snapshot);
        check(web_gpu_queue_program_snapshot(QUEUE,2)==0);
        invoke("nvnDeviceFinalizeCommandHandle",DEVICE,program_list,0,0);
        invoke("nvnDeviceFinalizeCommandHandle",DEVICE,changed_list,0,0);
        check(web_gpu_queue_program_snapshot(QUEUE,0)==new_snapshot);
        check(web_gpu_queue_program_snapshot(QUEUE,1)==old_snapshot);
        invoke("nvnCommandBufferBeginRecording",COMMANDS,0,0,0);
        invoke("nvnCommandBufferBindProgram",COMMANDS,0,1,0);
        uint64_t unbind_list=invoke("nvnCommandBufferEndRecording",COMMANDS,0,0,0);
        unbound=99;
        check(web_gpu_command_program_binding(unbind_list,0,&stages,&unbound)&&stages==1&&unbound==0);
        wr64(CONTROL+512,unbind_list);invoke("nvnQueueSubmitCommands",QUEUE,1,CONTROL+512,0);
        check(web_gpu_queue_program_snapshot(QUEUE,0)==0);
        check(web_gpu_queue_program_snapshot(QUEUE,1)==old_snapshot);
        invoke("nvnDeviceFinalizeCommandHandle",DEVICE,unbind_list,0,0);
        invoke("nvnCommandBufferFinalize",COMMANDS,0,0,0);
        invoke("nvnBufferBuilderSetDevice",BUFFER_BUILDER,DEVICE,0,0);
        invoke("nvnBufferBuilderSetDefaults",BUFFER_BUILDER,0,0,0);
        check(invoke("nvnBufferBuilderGetDevice",BUFFER_BUILDER,0,0,0)==DEVICE);
        invoke("nvnBufferBuilderSetStorage",BUFFER_BUILDER,POOL_OBJECT,128,256);
        uint8_t builder_copy[64];web_read(BUFFER_BUILDER,builder_copy,64);web_write(COPIED_BUILDER,builder_copy,64);
        check(invoke("nvnBufferBuilderGetMemoryPool",COPIED_BUILDER,0,0,0)==POOL_OBJECT);
        check(invoke("nvnBufferInitialize",BUFFER_OBJECT,COPIED_BUILDER,0,0)==1);
        check(invoke("nvnBufferMap",BUFFER_OBJECT,0,0,0)==STORAGE+128);
        check(invoke("nvnBufferGetAddress",BUFFER_OBJECT,0,0,0)==STORAGE+128);
        check(invoke("nvnBufferGetSize",BUFFER_OBJECT,0,0,0)==256);
        uint32_t cpu_value=0x12345678,gpu_value=0x9abcdef0;
        wr32(STORAGE+132,cpu_value);memcpy(expected+132,&cpu_value,4);
        invoke("nvnBufferFlushMappedRange",BUFFER_OBJECT,4,4,0);
        uint32_t handle=0;uint64_t offset=0;
        check(web_gpu_resolve(STORAGE+128,256,&handle,&offset)&&offset==128);
        check(!web_gpu_request(2,handle,offset+12,4,&gpu_value,4).status);
        check(rd32(STORAGE+140)!=gpu_value);
        invoke("nvnBufferInvalidateMappedRange",BUFFER_OBJECT,12,4,0);
        check(rd32(STORAGE+140)==gpu_value);memcpy(expected+140,&gpu_value,4);
        check(!web_gpu_request(3,handle,0,sizeof readback,readback,sizeof readback).status);
        check(!memcmp(expected,readback,sizeof expected));
        invoke("nvnBufferBuilderSetStorage",COPIED_BUILDER,POOL_OBJECT,81916,8);
        check(invoke("nvnBufferInitialize",BAD_BUFFER,COPIED_BUILDER,0,0)==0);
        invoke("nvnBufferFinalize",BUFFER_OBJECT,0,0,0);
        invoke("nvnTexturePoolFinalize",TEXTURES,0,0,0);
        invoke("nvnSamplerPoolFinalize",SAMPLERS,0,0,0);
        invoke("nvnMemoryPoolFinalize",POOL_OBJECT,0,0,0);
        check(web_gpu_request(3,handle,0,4,readback,4).status!=0);
        check(web_gpu_request(1,0,0,0,NULL,0).status!=0);
        WebGpuReply replacement=web_gpu_request(1,0,0,64,NULL,0);
        check(!replacement.status&&replacement.value>handle);
        check(!web_gpu_request(4,replacement.value,0,0,NULL,0).status);
        // GPU-only pools expose a GPU address, never a fake CPU mapping.
        invoke("nvnMemoryPoolBuilderSetStorage",POOL_BUILDER,0,4096,0);
        invoke("nvnMemoryPoolBuilderSetFlags",POOL_BUILDER,1,0,0);
        check(invoke("nvnMemoryPoolInitialize",POOL_OBJECT,POOL_BUILDER,0,0)==1);
        check(invoke("nvnMemoryPoolMap",POOL_OBJECT,0,0,0)==0);
        invoke("nvnBufferBuilderSetStorage",BUFFER_BUILDER,POOL_OBJECT,0,16);
        check(invoke("nvnBufferInitialize",BUFFER_OBJECT,BUFFER_BUILDER,0,0)==1);
        check(invoke("nvnBufferMap",BUFFER_OBJECT,0,0,0)==0);
        uint64_t gpu_address=invoke("nvnBufferGetAddress",BUFFER_OBJECT,0,0,0);
        check(gpu_address!=0);
        check(web_gpu_resolve(gpu_address,16,&handle,&offset)&&offset==0);
        check(!web_gpu_request(3,handle,0,16,readback,16).status);
        unsigned zero=1;for(unsigned i=0;i<16;i++)if(readback[i])zero=0;check(zero);
#ifdef WEB_GPU_WITH_SDK
        /* CPU_NO_ACCESS source: populate the actual GPU buffer, then require
         * texture initialization to read that storage through the GPU bridge. */
        TexLayout gpu_source_layout;check(tex_layout(&gpu_source_layout,1,0x25,4,4,1,1,0,0,-1)==0);
        for(unsigned i=0;i<64;i++)linear[i]=(uint8_t)(i*19+7);memset(tiled,0,sizeof tiled);
        tex_swizzle_level(&gpu_source_layout,0,linear,tiled);
        check(!web_gpu_request(2,handle,0,gpu_source_layout.size,tiled,gpu_source_layout.size).status);
        invoke("nvnTextureBuilderSetDefaults",TEXB,0,0,0);invoke("nvnTextureBuilderSetDevice",TEXB,DEVICE,0,0);
        invoke("nvnTextureBuilderSetSize2D",TEXB,4,4,0);invoke("nvnTextureBuilderSetFormat",TEXB,0x25,0,0);
        invoke("nvnTextureBuilderSetStorage",TEXB,POOL_OBJECT,0,0);
        check(invoke("nvnTextureInitialize",TEXTURE,TEXB,0,0)==1);
        uint32_t gpu_source_texture=0;check(web_gpu_texture_handle(TEXTURE,&gpu_source_texture));
        check(!web_gpu_request(16,gpu_source_texture,0,0,texture_readback,64).status);
        check(!memcmp(linear,texture_readback,64));
        const uint64_t present_builder=BASE+0x5600,present_window=BASE+0x5680,present_textures=BASE+0x5700;
        wr64(present_textures,TEXTURE);
        invoke("nvnWindowBuilderSetDevice",present_builder,DEVICE,0,0);
        invoke("nvnWindowBuilderSetDefaults",present_builder,0,0,0);
        invoke("nvnWindowBuilderSetTextures",present_builder,1,present_textures,0);
        check(invoke("nvnWindowInitialize",present_window,present_builder,0,0)==1);
        invoke("nvnQueuePresentTexture",QUEUE,present_window,0,0);
        /* invoke supplies x0..x3; SetCrop's height is its fifth argument. */
        context.x[4]=2;
        invoke("nvnWindowSetCrop",present_window,1,1,2);
        invoke("nvnQueuePresentTexture",QUEUE,present_window,0,0);
        if(!setjmp(expected_stop)) {
            trap_expected=1;invoke("nvnQueuePresentTexture",QUEUE,present_window,1,0);trap_expected=0;check(0);
        }else{trap_expected=0;check(strstr(error,"invalid NVN present texture index")!=NULL);error[0]=0;}
        invoke("nvnWindowFinalize",present_window,0,0,0);
        invoke("nvnTextureFinalize",TEXTURE,0,0,0);
#endif
        invoke("nvnBufferFinalize",BUFFER_OBJECT,0,0,0);
        invoke("nvnMemoryPoolFinalize",POOL_OBJECT,0,0,0);
        invoke("nvnQueueFenceSync",QUEUE,SYNC,0,1);
        check(invoke("nvnSyncWait",SYNC,UINT64_MAX,0,0)<=1);
        check(invoke("nvnSyncWait",SYNC,0,0,0)==0);
        check(invoke("nvnQueueWaitSync",QUEUE,SYNC,0,0)==1);
        invoke("nvnQueueFenceSync",QUEUE,SYNC,1,0);
        check(invoke("nvnSyncWait",SYNC,UINT64_C(1)<<40,0,0)<=1);
        invoke("nvnSyncFinalize",SYNC,0,0,0);
        check(invoke("nvnSyncWait",SYNC,0,0,0)==3);
        check(invoke("nvnSyncInitialize",SYNC,DEVICE,0,0)==1);
        check(invoke("nvnSyncWait",SYNC,0,0,0)==3);
        const uint64_t WIN_BUILDER=BASE+0x5400,WIN_OBJECT=BASE+0x5480,WIN_TEXS=BASE+0x5500,WIN_IDX=BASE+0x5540;
        wr64(WIN_TEXS,0x7100007800ULL);wr64(WIN_TEXS+8,0x7100007880ULL);wr64(WIN_TEXS+16,0x7100007900ULL);
        invoke("nvnWindowBuilderSetDevice",WIN_BUILDER,DEVICE,0,0);
        invoke("nvnWindowBuilderSetDefaults",WIN_BUILDER,0,0,0);
        check(invoke("nvnWindowBuilderGetDevice",WIN_BUILDER,0,0,0)==DEVICE);
        invoke("nvnWindowBuilderSetNativeWindow",WIN_BUILDER,0x7600000100ULL,0,0);
        invoke("nvnWindowBuilderSetTextures",WIN_BUILDER,3,WIN_TEXS,0);
        check(invoke("nvnWindowBuilderGetNumTextures",WIN_BUILDER,0,0,0)==3);
        check(invoke("nvnWindowInitialize",WIN_OBJECT,WIN_BUILDER,0,0)==1);
        wr32(WIN_IDX,99);
        check(invoke("nvnWindowAcquireTexture",WIN_OBJECT,SYNC,WIN_IDX,0)==0);
        check(rd32(WIN_IDX)==0);
        check(invoke("nvnSyncWait",SYNC,UINT64_MAX,0,0)<=1);
        invoke("nvnWindowSetPresentInterval",WIN_OBJECT,1,0,0);
        check(invoke("nvnWindowGetPresentInterval",WIN_OBJECT,0,0,0)==1);
        invoke("nvnWindowFinalize",WIN_OBJECT,0,0,0);
        invoke("nvnSyncFinalize",SYNC,0,0,0);
        check(invoke("nvnQueueGetError",QUEUE,0,0,0)==0);
        invoke("nvnQueueFinalize",QUEUE,0,0,0);
        invoke("nvnDeviceFinalize",DEVICE,0,0,0);
    } else failures++;
    web_memory_bind(NULL);gm_destroy(&memory);
    snprintf(report,sizeof report,"{\"ok\":%s,\"checks\":%u,\"failures\":%u,\"firstFailureLine\":%u,\"error\":\"%s\",\"textureFormat\":%u,\"textureStorage\":%llu}",failures?"false":"true",checks,failures,first_failure_line,error,last_texture_format,(unsigned long long)last_texture_storage);
    publish();return NULL;
}
int gpu_probe_start(void) {return pthread_create(&driver,NULL,run,NULL);}
int gpu_probe_join(void) {return pthread_join(driver,NULL);}
