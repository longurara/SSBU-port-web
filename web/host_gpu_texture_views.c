/* Native portable view encoding; output flags and int32 output widths match
 * the native frontend. WebGPU binding resolves these overrides per texture. */
#include "host_gpu_texture.h"
#include <string.h>
int web_gpu_texture_view_import(Ctx *c,const char *name) {
    if(strncmp(name,"nvnTextureView",14))return 0;
    const char *operation=name+14;uint64_t object=c->x[0];
    if(!strcmp(operation,"SetDefaults")){WebGpuTextureView view={0};view.set=0x80;web_write(object,&view,sizeof view);return 1;}
    if(!strcmp(operation,"Compare")) {
        if(!object||!c->x[1]){c->x[0]=0;return 1;}
        WebGpuTextureView a,b;web_read(object,&a,sizeof a);web_read(c->x[1],&b,sizeof b);c->x[0]=!memcmp(&a,&b,sizeof a);return 1;
    }
    int set=!strncmp(operation,"Set",3),get=!strncmp(operation,"Get",3);
    if(!set&&!get)return 0;
    const char *field=operation+3;const char *known[]={"Levels","Layers","Format","Swizzle","DepthStencilMode","Target"};
    unsigned index=0;while(index<6&&strcmp(field,known[index]))index++;if(index==6)return 0;
    WebGpuTextureView view;web_read(object,&view,sizeof view);
    const unsigned masks[]={0x81,0x82,0x84,0x88,0x90,0xa0};
    if(set) {
        if(index==0){view.lvl0=(int16_t)c->x[1];view.nlvl=(int16_t)c->x[2];}
        else if(index==1){view.lay0=(int16_t)c->x[1];view.nlay=(int16_t)c->x[2];}
        else if(index==2)view.format=(uint16_t)c->x[1];
        else if(index==3)for(unsigned i=0;i<4;i++)view.swz[i]=(uint8_t)c->x[i+1];
        else if(index==4)view.dsmode=(uint8_t)c->x[1];
        else view.target=(uint8_t)c->x[1];
        view.set|=(uint8_t)masks[index];web_write(object,&view,sizeof view);
    } else {
        if(index<2) {
            if(c->x[1])wr32(c->x[1],(uint32_t)(int32_t)(index==0?view.lvl0:view.lay0));
            if(c->x[2])wr32(c->x[2],(uint32_t)(int32_t)(index==0?view.nlvl:view.nlay));
        } else if(index==3){for(unsigned i=0;i<4;i++)if(c->x[i+1])wr32(c->x[i+1],view.swz[i]);}
        else if(c->x[1])wr32(c->x[1],index==2?view.format:index==4?view.dsmode:view.target);
        c->x[0]=(view.set&masks[index])==masks[index];
    }
    return 1;
}
