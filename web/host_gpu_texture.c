#include "host_gpu_texture.h"
#include "host_gpu_bridge.h"
#include <stddef.h>
#include <string.h>

int web_gpu_texture_import(Ctx *c,const char *name) {
    if(web_gpu_texture_object_import(c,name))return 1;
    if(web_gpu_texture_view_import(c,name))return 1;
    if(strncmp(name,"nvnTextureBuilder",17))return 0;
    const char *operation=name+17;uint64_t address=c->x[0];
    if(!strcmp(operation,"SetDevice")) {
        if(!c->x[1]||c->x[1]!=web_gpu_device_address())guest_trap("invalid texture-builder device",c->x[1]);
        wr64(address+96,c->x[1]);return 1;
    }
    if(!strcmp(operation,"GetDevice")){c->x[0]=rd64(address+96);return 1;}
    if(!strcmp(operation,"SetDefaults")) {
        WebGpuTextureBuilder b={0};b.target=b.levels=1;
        for(unsigned i=0;i<4;i++)b.swz[i]=(uint8_t)(i+2);
        web_write(address,&b,sizeof b);return 1;
    }
    const char *fields[]={"Flags","Target","Format","Width","Height","Depth","Levels","Samples","DepthStencilMode","GLTextureName"};
    const unsigned offsets[]={offsetof(WebGpuTextureBuilder,flags),offsetof(WebGpuTextureBuilder,target),offsetof(WebGpuTextureBuilder,format),
      offsetof(WebGpuTextureBuilder,width),offsetof(WebGpuTextureBuilder,height),offsetof(WebGpuTextureBuilder,depth),offsetof(WebGpuTextureBuilder,levels),
      offsetof(WebGpuTextureBuilder,samples),offsetof(WebGpuTextureBuilder,dsmode),offsetof(WebGpuTextureBuilder,glname)};
    int set=!strncmp(operation,"Set",3),get=!strncmp(operation,"Get",3);
    if(set||get)for(unsigned i=0;i<sizeof offsets/sizeof *offsets;i++)if(!strcmp(operation+3,fields[i])) {
        if(set)wr32(address+offsets[i],(uint32_t)c->x[1]);else c->x[0]=rd32(address+offsets[i]);return 1;
    }
    const char *known[]={"SetSize1D","SetSize2D","SetSize3D","SetStorage","GetMemoryPool","GetMemoryOffset","SetStride","GetStride",
      "SetPackagedTextureData","GetPackagedTextureData","SetPackagedTextureLayout","GetPackagedTextureLayout","SetSwizzle","GetSwizzle",
      "GetStorageSize","GetStorageAlignment","GetStorageClass","GetRawStorageClass","GetSparseTileLayout","GetZCullStorageSize"};
    int found=0;for(unsigned i=0;i<sizeof known/sizeof *known;i++)if(!strcmp(operation,known[i]))found=1;
    if(!found)return 0;
    WebGpuTextureBuilder b;web_read(address,&b,sizeof b);
    if(!strcmp(operation,"SetSize1D")||!strcmp(operation,"SetSize2D")||!strcmp(operation,"SetSize3D")) {
        b.width=(int32_t)c->x[1];b.height=strcmp(operation,"SetSize1D")?(int32_t)c->x[2]:1;
        b.depth=!strcmp(operation,"SetSize3D")?(int32_t)c->x[3]:1;
    } else if(!strcmp(operation,"SetStorage")){b.pool=c->x[1];b.offset=c->x[2];}
    else if(!strcmp(operation,"GetMemoryPool"))c->x[0]=b.pool;
    else if(!strcmp(operation,"GetMemoryOffset"))c->x[0]=b.offset;
    else if(!strcmp(operation,"SetStride"))b.stride=(int64_t)c->x[1];
    else if(!strcmp(operation,"GetStride"))c->x[0]=(uint64_t)b.stride;
    else if(!strcmp(operation,"SetPackagedTextureData"))b.packaged=c->x[1];
    else if(!strcmp(operation,"GetPackagedTextureData"))c->x[0]=b.packaged;
    else if(!strcmp(operation,"SetPackagedTextureLayout")){if(c->x[1]){web_read(c->x[1],b.layout,8);b.has_layout=1;}}
    else if(!strcmp(operation,"GetPackagedTextureLayout")){if(c->x[1])web_write(c->x[1],b.layout,8);}
    else if(!strcmp(operation,"SetSwizzle")){for(unsigned i=0;i<4;i++)b.swz[i]=(uint8_t)c->x[i+1];}
    else if(!strcmp(operation,"GetSwizzle")){for(unsigned i=0;i<4;i++)if(c->x[i+1])wr32(c->x[i+1],b.swz[i]);}
    else if(!strcmp(operation,"GetZCullStorageSize"))c->x[0]=0;
    else {
        unsigned function=!strcmp(operation,"GetStorageSize")?0x428760:!strcmp(operation,"GetStorageAlignment")?0x4278b0:
            !strcmp(operation,"GetStorageClass")?0x427a10:!strcmp(operation,"GetRawStorageClass")?0x427b30:0x4290b0;
        uint32_t output[4];uint64_t result=web_nvn_layout_query(c,&b,function,output);
        if(function==0x4290b0){if(c->x[1])web_write(c->x[1],output,16);}else c->x[0]=result;
    }
    if(set)web_write(address,&b,sizeof b);return 1;
}
