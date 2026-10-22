/* NVN compatibility entry point registry. Each unimplemented command stops
 * explicitly; having a WebGPU adapter does not imply a working game renderer. */
#include "runtime.h"
#include "host_gpu_bridge.h"
#include "host_gpu_sampler.h"
#include "host_gpu_texture.h"
#include <emscripten/emscripten.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define GPU_FUNCTION_BASE 0x7a00000000ULL
#define GPU_FUNCTION_LIMIT 2048u
typedef struct { uint64_t allocate, release, resize, user; } GraphicsAllocator;
static GraphicsAllocator allocators[2];
static uint64_t graphics_workspace, graphics_workspace_size;
static uint64_t device_address;
static uint32_t device_flags;
static atomic_uint origin_mode=1, depth_mode=1;
uint32_t web_gpu_origin_mode(void){return atomic_load(&origin_mode);}
static char functions[GPU_FUNCTION_LIMIT][128];
static unsigned function_count;
static pthread_mutex_t registry = PTHREAD_MUTEX_INITIALIZER;

static int gpu_ready(void) {
    return MAIN_THREAD_EM_ASM_INT({return Module['webgpuDevice'] ? 1 : 0;});
}
uint64_t web_gpu_device_address(void) {return device_address;}
static int gpu_capability(unsigned name) {
    return MAIN_THREAD_EM_ASM_INT({
        const values=Module['webgpuBackend']?.capabilities;
        return values && $0<values.length ? values[$0] : -1;
    },name);
}
static void string_at(uint64_t address,char *out,size_t size) {
    for(size_t i=0;i<size;i++){out[i]=(char)rd8(address+i);if(!out[i])return;}
    guest_trap("NVN function name too long",address);
}
static uint64_t register_function(uint64_t string) {
    char name[128];string_at(string,name,sizeof name);
    pthread_mutex_lock(&registry);
    for(unsigned i=0;i<function_count;i++)if(!strcmp(functions[i],name)){pthread_mutex_unlock(&registry);return GPU_FUNCTION_BASE+(uint64_t)i*256;}
    if(function_count==GPU_FUNCTION_LIMIT){pthread_mutex_unlock(&registry);guest_trap("NVN function registry exhausted",string);}
    unsigned index=function_count;strcpy(functions[index],name);function_count++;
    pthread_mutex_unlock(&registry);return GPU_FUNCTION_BASE+(uint64_t)index*256;
}
int web_gpu_indirect(Ctx *c,uint64_t address) {
    if(address<GPU_FUNCTION_BASE || address>=GPU_FUNCTION_BASE+GPU_FUNCTION_LIMIT*256 || (address&255))return 0;
    unsigned index=(unsigned)((address-GPU_FUNCTION_BASE)/256);
    pthread_mutex_lock(&registry);
    unsigned count=function_count;
    pthread_mutex_unlock(&registry);
    if(index>=count)guest_trap("invalid NVN function handle",address);
    const char *name=functions[index];
    if(!strcmp(name,"nvnSetExternalDebuggerEnabledNVX")) {
        if(c->x[0])guest_trap("external NVN debugger unavailable in browser",c->x[30]);
        return 1;
    }
    if(!strcmp(name,"nvnDeviceBuilderSetDefaults")) {
        for(unsigned i=0;i<64;i+=8)wr64(c->x[0]+i,0);
        return 1;
    }
    if(!strcmp(name,"nvnDeviceBuilderSetFlags")){wr32(c->x[0],(uint32_t)c->x[1]);return 1;}
    if(!strcmp(name,"nvnDeviceBuilderGetFlags")){c->x[0]=rd32(c->x[0]);return 1;}
    if(!strcmp(name,"nvnDeviceInitialize")) {
        if(!gpu_ready())guest_trap("WebGPU device lost or unavailable",c->x[30]);
        if(!c->x[0] || !c->x[1]){c->x[0]=0;return 1;}
        if(device_address)guest_trap("multiple NVN devices unsupported",c->x[30]);
        device_flags=rd32(c->x[1]);
        rd8(c->x[0]);device_address=c->x[0];c->x[0]=1;return 1;
    }
    if(!strcmp(name,"nvnDeviceFinalize")) {
        if(c->x[0]!=device_address)guest_trap("invalid NVN device",c->x[0]);
        device_address=0;device_flags=0;return 1;
    }
    if(!strcmp(name,"nvnDeviceSetWindowOriginMode") || !strcmp(name,"nvnDeviceSetDepthMode")) {
        if(c->x[0]!=device_address || c->x[1]>1)guest_trap("invalid NVN clip mode",c->x[30]);
        if(!strcmp(name,"nvnDeviceSetWindowOriginMode"))origin_mode=(uint32_t)c->x[1];else depth_mode=(uint32_t)c->x[1];
        return 1;
    }
    if(!strcmp(name,"nvnDeviceGetWindowOriginMode")){c->x[0]=origin_mode;return 1;}
    if(!strcmp(name,"nvnDeviceGetDepthMode")){c->x[0]=depth_mode;return 1;}
    if(!strcmp(name,"nvnDeviceRegisterFastClearDepth") || !strcmp(name,"nvnDeviceRegisterFastClearColor") ||
       !strcmp(name,"nvnDeviceRegisterFastClearColori") || !strcmp(name,"nvnDeviceRegisterFastClearColorui")) {
        // Registration is an optional native compression optimisation. WebGPU
        // has no NVN fast-clear registry; report registration failure honestly.
        if(c->x[0]!=device_address)guest_trap("invalid NVN device",c->x[0]);
        c->x[0]=0;return 1;
    }
    if(!strcmp(name,"nvnDeviceApplyDeferredFinalizes") || !strcmp(name,"nvnDeviceInstallDebugCallback") ||
       !strcmp(name,"nvnDeviceSetIntermediateShaderCache") || !strcmp(name,"nvnDeviceWalkDebugDatabase")) {
        if(c->x[0]!=device_address)guest_trap("invalid NVN device",c->x[0]);
        return 1;
    }
    if(!strcmp(name,"nvnDeviceSetDebugLabel")) {
        if(c->x[0]!=device_address)guest_trap("invalid NVN device",c->x[0]);
        if(c->x[1])(void)rd8(c->x[1]);
        return 1;
    }
    if(!strcmp(name,"nvnDeviceIsExternalDebuggerAttached")) {
        if(c->x[0]!=device_address)guest_trap("invalid NVN device",c->x[0]);
        c->x[0]=0;return 1;
    }
    if(!strcmp(name,"nvnDeviceGenerateDebugDomainId")) {
        static atomic_uint next_domain=1;
        if(c->x[0]!=device_address)guest_trap("invalid NVN device",c->x[0]);
        c->x[0]=atomic_fetch_add(&next_domain,1);return 1;
    }
    if(!strcmp(name,"nvnDeviceGetCurrentTimestampInNanoseconds")) {
        if(c->x[0]!=device_address)guest_trap("invalid NVN device",c->x[0]);
        c->x[0]=(uint64_t)(emscripten_get_now()*1000000.0);return 1;
    }
    if(!strcmp(name,"nvnDeviceGetTimestampInNanoseconds")) {
        if(c->x[0]!=device_address)guest_trap("invalid NVN device",c->x[0]);
        c->x[0]=c->x[1]?rd64(c->x[1]+8):0;return 1;
    }
    if(!strcmp(name,"nvnSyncCreateGLSync") || !strcmp(name,"nvnSyncInitializeFromFencedGLSync")) {
        c->x[0]=0;return 1;
    }
    if(!strcmp(name,"nvnMemoryPoolMapVirtual") || !strcmp(name,"nvnProgramSetSubroutineLinkage")) {
        c->x[0]=1;return 1;
    }
    if(!strcmp(name,"nvnTextureInvalidateTexels") || !strcmp(name,"nvnTextureReadTexels") || !strcmp(name,"nvnTextureReadTexelsStrided")) {
        return 1;
    }
    if(!strcmp(name,"nvnMultisampleStateGetSampleLocationsGrid")) {
        (void)rd8(c->x[0]);
        if(c->x[1])wr32(c->x[1],1);
        if(c->x[2])wr32(c->x[2],1);
        return 1;
    }
    if(!strcmp(name,"nvnDeviceGetProcAddress")){c->x[0]=register_function(c->x[1]);return 1;}
    if(!strcmp(name,"nvnDeviceGetInteger")) {
        if(c->x[0] && c->x[0]!=device_address)guest_trap("invalid NVN device",c->x[0]);
        int value=c->x[1]<=0xffffffff ? gpu_capability((unsigned)c->x[1]) : -1;
        if(value<0)guest_trap("unknown NVN device capability",c->x[1]);
        if(c->x[2])wr32(c->x[2],(uint32_t)value);return 1;
    }
    if(!strcmp(name,"nvnDeviceGetTextureHandle")||!strcmp(name,"nvnDeviceGetTexelFetchHandle")||!strcmp(name,"nvnDeviceGetImageHandle")||
       !strcmp(name,"nvnDeviceGetSeparateTextureHandle")||!strcmp(name,"nvnDeviceGetSeparateSamplerHandle")) {
        if(!c->x[0]||c->x[0]!=device_address)guest_trap("invalid descriptor-handle device",c->x[0]);
        if(!strcmp(name,"nvnDeviceGetSeparateSamplerHandle")){c->x[0]=(uint64_t)(int64_t)(int32_t)(uint32_t)c->x[1]<<20;return 1;}
        uint32_t value=(uint32_t)c->x[1];
        if(!strcmp(name,"nvnDeviceGetTextureHandle"))value|=(uint32_t)c->x[2]<<20;
        c->x[0]=(uint64_t)(int64_t)(int32_t)value;
        if(strcmp(name,"nvnDeviceGetImageHandle"))c->x[0]|=0x100000000ULL;
        return 1;
    }
    if(web_gpu_resources_import(c,name))return 1;
    if(web_gpu_commands_import(c,name))return 1;
    if(web_gpu_states_import(c,name))return 1;
    if(web_gpu_sampler_import(c,name))return 1;
    if(web_gpu_programs_import(c,name))return 1;
    if(web_gpu_texture_import(c,name))return 1;
    char message[160];snprintf(message,sizeof message,"unimplemented NVN: %s",name);guest_trap(message,c->x[30]);return 0;
}
int web_gpu_import(Ctx *c,const char *name) {
    int allocator=-1;
    if(!strcmp(name,"_ZN2nv20SetGraphicsAllocatorEPFPvmmS0_EPFvS0_S0_EPFS0_S0_mS0_ES0_"))allocator=0;
    if(!strcmp(name,"_ZN2nv28SetGraphicsDevtoolsAllocatorEPFPvmmS0_EPFvS0_S0_EPFS0_S0_mS0_ES0_"))allocator=1;
    if(allocator>=0){allocators[allocator]=(GraphicsAllocator){c->x[0],c->x[1],c->x[2],c->x[3]};return 1;}
    if(!strcmp(name,"_ZN2nv18InitializeGraphicsEPvm")) {
        if(!gpu_ready())guest_trap("WebGPU device not attached; use browser boot",c->x[30]);
        if(!c->x[1])guest_trap("empty graphics workspace",c->x[0]);
        rd8(c->x[0]);rd8(c->x[0]+c->x[1]-1);
        graphics_workspace=c->x[0];graphics_workspace_size=c->x[1];return 1;
    }
    if(!strcmp(name,"nvnBootstrapLoader")) {
        if(!graphics_workspace || !graphics_workspace_size)guest_trap("graphics not initialised",c->x[30]);
        c->x[0]=register_function(c->x[0]);return 1;
    }
    return 0;
}
