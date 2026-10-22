/* Immutable NVN target/view/scissor/color metadata. Queue state persists across
 * submissions, while all GPU actions run under that queue's submission lock. */
#include "host_gpu_render.h"
#include "host_gpu_texture.h"
#include "host_gpu_bridge.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
typedef struct {WebGpuTextureRef *texture;WebGpuTextureView view;} Target;
typedef struct {atomic_uint references;Target targets[9];} Targets;
struct WebGpuRenderRecord {unsigned operation,first,count,index,mask,kind;Targets *targets;int32_t scissors[16][4];uint32_t color[4];};
struct WebGpuRenderState {Targets *targets;int32_t scissors[16][4];unsigned scissors_set;};
static void release_targets(Targets *targets) {
    if(targets&&atomic_fetch_sub(&targets->references,1)==1){for(unsigned i=0;i<9;i++)if(targets->targets[i].texture)web_gpu_texture_release(targets->targets[i].texture);free(targets);}
}
void web_gpu_render_release(WebGpuRenderRecord *record){if(record){release_targets(record->targets);free(record);}}
void web_gpu_render_state_release(WebGpuRenderState *state){if(state){release_targets(state->targets);free(state);}}
const char *web_gpu_render_draw_targets(const WebGpuRenderState *state,WebGpuDrawTarget output[9],int32_t scissor[4],uint32_t extent[2]) {
    memset(output,0,9*sizeof *output);extent[0]=extent[1]=0;
    if(!state||!state->targets)return "draw has no NVN render targets";
    if(web_gpu_origin_mode()!=1)return "lower-left NVN draw origin needs lowering";
    for(unsigned i=0;i<9;i++) {
        const Target *target=&state->targets->targets[i];if(!target->texture)continue;
        WebGpuTextureBuilder builder;uint32_t handle;web_gpu_texture_snapshot(target->texture,&builder,&handle);
        const WebGpuTextureView *v=&target->view;
        int mip=(v->set&0x81)==0x81?v->lvl0:0,layer=(v->set&0x82)==0x82?v->lay0:0;
        int layers=builder.target==8?6:builder.target==3||builder.target==4||builder.target==9?builder.depth:1;
        int count=(v->set&0x82)==0x82&&v->nlay>0?v->nlay:layers-layer;
        if((v->set&0xa0)==0xa0&&v->target==1)count=1;
        if(mip<0||mip>=builder.levels||layer<0||count<1||count>layers-layer)return "invalid NVN draw target subresource";
        output[i]=(WebGpuDrawTarget){handle,(uint32_t)mip,(uint32_t)layer,(v->set&0x84)==0x84?v->format:0,(uint32_t)count};
        if(!extent[0]){extent[0]=(uint32_t)builder.width>>mip;extent[1]=(uint32_t)builder.height>>mip;if(!extent[0])extent[0]=1;if(!extent[1])extent[1]=1;}
    }
    if(!extent[0])return "draw has no NVN render target";
    if(state->scissors_set&1)memcpy(scissor,state->scissors[0],16);else{scissor[0]=scissor[1]=0;scissor[2]=extent[0];scissor[3]=extent[1];}
    int64_t x=scissor[0],y=scissor[1],right=x+(scissor[2]>0?scissor[2]:0),bottom=y+(scissor[3]>0?scissor[3]:0);
    if(x<0)x=0;if(y<0)y=0;if(x>extent[0])x=extent[0];if(y>extent[1])y=extent[1];
    if(right<x)right=x;if(bottom<y)bottom=y;if(right>extent[0])right=extent[0];if(bottom>extent[1])bottom=extent[1];
    scissor[0]=(int32_t)x;scissor[1]=(int32_t)y;scissor[2]=(int32_t)(right-x);scissor[3]=(int32_t)(bottom-y);return NULL;
}
int web_gpu_render_known(const char *name){return !strcmp(name,"SetRenderTargets")||!strcmp(name,"SetScissor")||!strcmp(name,"SetScissors")||!strcmp(name,"ClearColor")||!strcmp(name,"ClearColori")||!strcmp(name,"ClearColorui")||!strcmp(name,"ClearDepthStencil");}
static int read_guest(uint64_t address,void *output,size_t size){return address&&gm_read(web_memory_current(),address,output,size)==GM_OK;}
WebGpuRenderRecord *web_gpu_render_record(Ctx *c,const char *name,const char **error) {
    *error="invalid NVN render command";
    WebGpuRenderRecord *record=calloc(1,sizeof *record);if(!record){*error="render command allocation failed";return NULL;}
    if(!strcmp(name,"SetRenderTargets")) {
        record->operation=1;
        if(c->x[1]>8){*error="too many NVN color targets";goto failed;}
        if(c->x[1]&&((c->x[2]&&c->x[2]>UINT64_MAX-c->x[1]*8)||(c->x[3]&&c->x[3]>UINT64_MAX-c->x[1]*8)))goto failed;
        record->targets=calloc(1,sizeof *record->targets);if(!record->targets)goto failed;
        atomic_init(&record->targets->references,1);
        for(unsigned i=0;i<9;i++) {
            uint64_t object=0,view=0;
            if(i<8){if(i>=c->x[1])continue;
                if((c->x[2]&&!read_guest(c->x[2]+i*8,&object,8))||(c->x[3]&&!read_guest(c->x[3]+i*8,&view,8)))goto failed;
            }else{object=c->x[4];view=c->x[5];}
            Target *target=&record->targets->targets[i];
            if(object&&!(target->texture=web_gpu_texture_hold(object))){*error="invalid NVN render-target texture";goto failed;}
            if(view&&!read_guest(view,&target->view,sizeof target->view))goto failed;
        }
    }else if(!strcmp(name,"SetScissor")||!strcmp(name,"SetScissors")) {
        record->operation=2;
        if(!strcmp(name,"SetScissor")){record->count=1;for(unsigned i=0;i<4;i++)record->scissors[0][i]=(int32_t)c->x[i+1];}
        else {
            if(c->x[1]>16||c->x[2]>16-c->x[1]){*error="invalid NVN scissor range";goto failed;}
            record->first=(unsigned)c->x[1];record->count=(unsigned)c->x[2];
            if(record->count&&!read_guest(c->x[3],record->scissors,record->count*16))goto failed;
        }
    }else if(!strcmp(name,"ClearDepthStencil")) {
        record->operation=4;record->kind=!!c->x[1];record->index=(uint32_t)c->x[2]&255;record->mask=(uint32_t)c->x[3]&255;
        float depth=c->v[0].f[0];
        if(!isfinite(depth)){*error="non-finite NVN clear depth";goto failed;}
        if(depth<0)depth=0;if(depth>1)depth=1;
        memcpy(record->color,&depth,4);
    }else {
        record->operation=3;
        if(c->x[1]>7||(c->x[3]&~15ULL)){*error="invalid NVN clear target or mask";goto failed;}
        record->index=(unsigned)c->x[1];record->mask=(unsigned)c->x[3];
        record->kind=!strcmp(name,"ClearColori")?1:!strcmp(name,"ClearColorui")?2:0;
        if(c->x[2]&&!read_guest(c->x[2],record->color,16))goto failed;
    }
    return record;
failed:web_gpu_render_release(record);return NULL;
}
static unsigned color_kind(int format) {
    if((format>=1&&format<=0x30)||(format>=0x6d&&format<=0x78)) {
        unsigned index=format>=0x6d?(unsigned)format-0x6d:((unsigned)format-1)%12;
        if(index==2||index==7||index==10)return 2;if(index==3||index==8||index==11)return 1;
    }
    return format==0x3e?2:0;
}
static const char *clear_depth(WebGpuRenderState *state,const WebGpuRenderRecord *record) {
    if((!record->kind&&!record->mask)||!state->targets||!state->targets->targets[8].texture)return NULL;
    if(!(state->scissors_set&1))return "NVN depth/stencil clear requires an explicit scissor";
    if(web_gpu_origin_mode()!=1)return "lower-left NVN clear origin needs validation";
    Target *target=&state->targets->targets[8];WebGpuTextureBuilder builder;uint32_t handle;
    web_gpu_texture_snapshot(target->texture,&builder,&handle);const WebGpuTextureView *view=&target->view;
    unsigned format=(view->set&0x84)==0x84?view->format:(unsigned)builder.format;
    if(format<0x31||format>0x36||format!=(unsigned)builder.format)return "invalid NVN depth/stencil clear format";
    if(builder.target!=1&&builder.target!=3&&builder.target!=4&&builder.target!=7&&builder.target!=8&&builder.target!=9)return "depth/stencil clear dimension not ported";
    if((view->set&0xa0)==0xa0&&view->target!=builder.target&&view->target!=1)return "depth/stencil clear view target not ported";
    int mip=(view->set&0x81)==0x81?view->lvl0:0,layer=(view->set&0x82)==0x82?view->lay0:0;
    int layers=builder.target==8?6:builder.target==3||builder.target==4||builder.target==9?builder.depth:1;
    int count=(view->set&0x82)==0x82&&view->nlay>0?view->nlay:layers-layer;
    if((view->set&0xa0)==0xa0&&view->target==1)count=1;
    if(mip<0||mip>=builder.levels||layer<0||count<1||count>layers-layer||count>4096)return "invalid depth/stencil clear subresource";
    int64_t width=(uint32_t)builder.width>>mip,height=(uint32_t)builder.height>>mip;if(!width)width=1;if(!height)height=1;
    int64_t x=state->scissors[0][0],y=state->scissors[0][1],w=state->scissors[0][2],h=state->scissors[0][3];
    if(w<0)w=0;if(h<0)h=0;int64_t right=x+w,bottom=y+h;
    if(x<0)x=0;if(y<0)y=0;if(x>width)x=width;if(y>height)y=height;
    if(right<x)right=x;if(bottom<y)bottom=y;if(right>width)right=width;if(bottom>height)bottom=height;
    size_t length=4+(size_t)count*48;uint8_t *packet=malloc(length);if(!packet)return "depth/stencil clear staging allocation failed";
    uint32_t n=(uint32_t)count;memcpy(packet,&n,4);
    for(int i=0;i<count;i++) {
        uint32_t fields[]={handle,(uint32_t)mip,(uint32_t)(layer+i),record->kind,record->color[0],record->index,record->mask,
                           (uint32_t)x,(uint32_t)y,(uint32_t)(right-x),(uint32_t)(bottom-y),format};
        memcpy(packet+4+(size_t)i*48,fields,sizeof fields);
    }
    WebGpuReply reply=web_gpu_request(25,0,0,length,packet,length);free(packet);
    return reply.status?"WebGPU depth/stencil clear failed":NULL;
}
const char *web_gpu_render_execute(WebGpuRenderState **state_pointer,const WebGpuRenderRecord *record) {
    WebGpuRenderState *state=*state_pointer;
    if(!state){state=calloc(1,sizeof *state);if(!state)return "render queue-state allocation failed";*state_pointer=state;}
    if(record->operation==1) {
        unsigned references=atomic_load(&record->targets->references);
        do{if(references==UINT32_MAX)return "too many render-target references";}while(!atomic_compare_exchange_weak(&record->targets->references,&references,references+1));
        release_targets(state->targets);state->targets=record->targets;return NULL;
    }
    if(record->operation==2) {
        for(unsigned i=0;i<record->count;i++){memcpy(state->scissors[record->first+i],record->scissors[i],16);state->scissors_set|=1u<<(record->first+i);}return NULL;
    }
    if(record->operation==4)return clear_depth(state,record);
    if(!record->mask||!state->targets||!state->targets->targets[record->index].texture)return NULL;
    if(!(state->scissors_set&1))return "NVN color clear requires an explicit scissor";
    if(web_gpu_origin_mode()!=1)return "lower-left NVN render origin needs validation";
    Target *target=&state->targets->targets[record->index];WebGpuTextureBuilder builder;uint32_t handle;
    web_gpu_texture_snapshot(target->texture,&builder,&handle);const WebGpuTextureView *view=&target->view;
    if((view->set&0xa0)==0xa0&&view->target!=builder.target&&view->target!=1)return "render-target view target reinterpretation not ported";
    uint32_t view_format=(view->set&0x84)==0x84?view->format:0;
    if(builder.target!=1&&builder.target!=3&&builder.target!=4&&builder.target!=7&&builder.target!=8&&builder.target!=9)return "render-target dimension not ported";
    int mip=(view->set&0x81)==0x81?view->lvl0:0,layer=(view->set&0x82)==0x82?view->lay0:0;
    int layers=builder.target==8?6:builder.target==3||builder.target==4||builder.target==9?builder.depth:1;
    int count=(view->set&0x82)==0x82&&view->nlay>0?view->nlay:layers-layer;
    if((view->set&0xa0)==0xa0&&view->target==1)count=1;
    if(mip<0||mip>=builder.levels||layer<0||layer>=layers||count<1||count>layers-layer||count>4096)return "invalid render-target mip/layer range";
    unsigned kind=color_kind(view_format?view_format:builder.format);if(record->kind&&record->kind!=kind)return "typed NVN clear does not match target format";
    int64_t width=(uint32_t)builder.width>>mip,height=(uint32_t)builder.height>>mip;if(!width)width=1;if(!height)height=1;
    int64_t x=state->scissors[0][0],y=state->scissors[0][1],w=state->scissors[0][2],h=state->scissors[0][3];
    if(w<0)w=0;if(h<0)h=0;int64_t right=x+w,bottom=y+h;
    if(x<0)x=0;if(y<0)y=0;if(x>width)x=width;if(y>height)y=height;
    if(right<x)right=x;if(bottom<y)bottom=y;if(right>width)right=width;if(bottom>height)bottom=height;
    size_t length=4+(size_t)count*60;uint8_t *packet=malloc(length);if(!packet)return "clear packet allocation failed";
    uint32_t n=(uint32_t)count;memcpy(packet,&n,4);
    for(int i=0;i<count;i++) {
        uint32_t fields[]={handle,(uint32_t)mip,(uint32_t)(layer+i),kind,record->mask,1,(uint32_t)x,(uint32_t)y,(uint32_t)(right-x),(uint32_t)(bottom-y)};
        memcpy(packet+4+(size_t)i*60,fields,40);memcpy(packet+44+(size_t)i*60,record->color,16);memcpy(packet+60+(size_t)i*60,&view_format,4);
    }
    WebGpuReply reply=web_gpu_request(22,0,0,length,packet,length);free(packet);
    return reply.status?"WebGPU color clear failed":NULL;
}
