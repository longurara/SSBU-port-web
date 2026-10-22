#include "runtime.h"
#include "host_gpu_bridge.h"
#include "host_gpu_sampler.h"
#include <string.h>

int web_gpu_sampler_import(Ctx *c,const char *name) {
    int builder=!strncmp(name,"nvnSamplerBuilder",17);
    if(!builder&&(strncmp(name,"nvnSampler",10)||!strncmp(name,"nvnSamplerPool",14)))return 0;
    const char *operation=name+(builder?17:10);uint64_t address=c->x[0];
    if(builder&&!strcmp(operation,"SetDefaults")) {
        uint64_t dev=rd64(address+48)==web_gpu_device_address()?web_gpu_device_address():0;
        uint8_t zero[96]={0};web_write(address,zero,sizeof zero);
        WebGpuSamplerParams p={0};p.min=p.mag=1;p.ws=p.wt=p.wr=7;p.lodmax=1000;p.aniso=1;p.cfunc=2;
        web_write(address,&p,sizeof p);wr64(address+48,dev);return 1;
    }
    if(builder&&!strcmp(operation,"SetDevice")) {
        if(c->x[1]!=web_gpu_device_address())guest_trap("invalid sampler-builder device",c->x[1]);
        wr64(address+48,c->x[1]);return 1;
    }
    if(builder&&!strcmp(operation,"GetDevice")){c->x[0]=rd64(address+48);return 1;}
    if(!builder&&!strcmp(operation,"Initialize")) {
        uint64_t source=c->x[1];
        if(!address||!source||rd64(source+48)!=web_gpu_device_address()){c->x[0]=0;return 1;}
        WebGpuSamplerParams p;web_read(source,&p,sizeof p);web_write(address,&p,sizeof p);c->x[0]=1;return 1;
    }
    if(!builder&&!strcmp(operation,"Compare")) {
        if(!address||!c->x[1]){c->x[0]=0;return 1;}
        WebGpuSamplerParams a,b;web_read(address,&a,sizeof a);web_read(c->x[1],&b,sizeof b);c->x[0]=!memcmp(&a,&b,sizeof a);return 1;
    }
    if(!builder&&!strcmp(operation,"GetDebugID")){c->x[0]=address;return 1;}
    // Values are copied into descriptor tables. There is no object-owned GPU
    // sampler to release here; physical samplers are materialized at binding.
    if(!builder&&(!strcmp(operation,"Finalize")||!strcmp(operation,"SetDebugLabel")))return 1;
    const char *known[]={"SetMinMagFilter","SetWrapMode","SetLodClamp","SetLodBias","SetCompare","SetMaxAnisotropy",
      "SetReductionFilter","SetLodSnap","SetBorderColor","SetBorderColori","SetBorderColorui",
      "GetMinMagFilter","GetWrapMode","GetLodClamp","GetLodBias","GetCompare","GetMaxAnisotropy","GetReductionFilter",
      "GetLodSnap","GetBorderColor","GetBorderColori","GetBorderColorui"};
    int found=0;for(unsigned i=0;i<sizeof known/sizeof *known;i++)if(!strcmp(operation,known[i]))found=1;
    int set=!strncmp(operation,"Set",3);if(!found||(set&&!builder))return 0;
    WebGpuSamplerParams p;web_read(address,&p,sizeof p);
    if(!strcmp(operation,"SetMinMagFilter")){p.min=(uint8_t)c->x[1];p.mag=(uint8_t)c->x[2];}
    else if(!strcmp(operation,"SetWrapMode")){p.ws=(uint8_t)c->x[1];p.wt=(uint8_t)c->x[2];p.wr=(uint8_t)c->x[3];}
    else if(!strcmp(operation,"SetLodClamp")){p.lodmin=c->v[0].f[0];p.lodmax=c->v[1].f[0];}
    else if(!strcmp(operation,"SetLodBias"))p.bias=c->v[0].f[0];
    else if(!strcmp(operation,"SetMaxAnisotropy"))p.aniso=c->v[0].f[0];
    else if(!strcmp(operation,"SetLodSnap"))p.lod_snap=c->v[0].f[0];
    else if(!strcmp(operation,"SetCompare")){p.cmode=(uint8_t)c->x[1];p.cfunc=(uint8_t)c->x[2];}
    else if(!strcmp(operation,"SetReductionFilter"))p.reduction=(uint8_t)c->x[1];
    else if(!strncmp(operation,"SetBorderColor",14)) {
        if(c->x[1])web_read(c->x[1],p.border,16);
        p.border_kind=!strcmp(operation,"SetBorderColorui")?1:!strcmp(operation,"SetBorderColori")?2:0;
    }
    else if(!strcmp(operation,"GetMinMagFilter")){if(c->x[1])wr32(c->x[1],p.min);if(c->x[2])wr32(c->x[2],p.mag);}
    else if(!strcmp(operation,"GetWrapMode")){if(c->x[1])wr32(c->x[1],p.ws);if(c->x[2])wr32(c->x[2],p.wt);if(c->x[3])wr32(c->x[3],p.wr);}
    else if(!strcmp(operation,"GetLodClamp")){if(c->x[1])web_write(c->x[1],&p.lodmin,4);if(c->x[2])web_write(c->x[2],&p.lodmax,4);}
    else if(!strcmp(operation,"GetLodBias"))c->v[0].f[0]=p.bias;
    else if(!strcmp(operation,"GetMaxAnisotropy"))c->v[0].f[0]=p.aniso;
    else if(!strcmp(operation,"GetLodSnap"))c->v[0].f[0]=p.lod_snap;
    else if(!strcmp(operation,"GetCompare")){if(c->x[1])wr32(c->x[1],p.cmode);if(c->x[2])wr32(c->x[2],p.cfunc);}
    else if(!strcmp(operation,"GetReductionFilter"))c->x[0]=p.reduction;
    else if(!strncmp(operation,"GetBorderColor",14)){if(c->x[1])web_write(c->x[1],p.border,16);}
    if(set)web_write(address,&p,sizeof p);
    return 1;
}
