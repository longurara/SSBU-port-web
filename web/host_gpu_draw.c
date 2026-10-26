/* Immutable graphics bindings and real draw packets. Resource bytes are read
 * at submission, while metadata and pool lifetimes are retained at recording. */
#include "host_gpu_draw.h"
#include "host_gpu_bridge.h"
#include "host_gpu_texture.h"
#include "host_gpu_sampler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

typedef struct {uint64_t address,size,pool,offset;} Range;
enum {ATTR=1,STREAM,VERTEX,UNIFORM,STORAGE,TEXTURE,BLEND,COLOR,MASK,POLYGON,DEPTH,MULTISAMPLE,
      VIEWPORT,DEPTH_RANGE,STENCIL_REF,STENCIL_READ,STENCIL_WRITE,BLEND_COLOR,ALPHA_REF,POINT_SIZE,
      SAMPLE_MASK,RASTER_DISCARD,DEPTH_CLAMP,RESTART,OFFSET,UNSUPPORTED,ARRAYS,ELEMENTS,DISPATCH};
struct WebGpuDrawRecord {unsigned op,stage,first,count,kind,range_count;uint64_t x[7];float f[4];uint8_t data[];};
#define RANGES(record) ((Range *)(void *)(record)->data)
struct WebGpuDrawState {
    uint32_t attributes[32],streams[32][2],channel_mask,sample_mask,stencil[6];
    Range vertex[32],uniform[6][16],storage[6][32];uint64_t textures[6][32];
    uint8_t blend[8][8],color[4],polygon[4],depth[8],multisample[24];
    float viewport[4],depth_range[2],blend_color[4],alpha_ref,point_size,offset[3];
    unsigned has_viewport,raster_discard,depth_clamp,restart,unsupported;
};
static unsigned operation(const char *name) {
    static const struct {const char *name;unsigned op;} names[]={
        {"BindVertexAttribState",ATTR},{"BindVertexStreamState",STREAM},{"BindVertexBuffer",VERTEX},{"BindVertexBuffers",VERTEX},
        {"BindUniformBuffer",UNIFORM},{"BindUniformBuffers",UNIFORM},{"BindStorageBuffer",STORAGE},{"BindStorageBuffers",STORAGE},
        {"BindTexture",TEXTURE},{"BindTextures",TEXTURE},{"BindBlendState",BLEND},{"BindColorState",COLOR},{"BindChannelMaskState",MASK},
        {"BindPolygonState",POLYGON},{"BindDepthStencilState",DEPTH},{"BindMultisampleState",MULTISAMPLE},
        {"SetViewport",VIEWPORT},{"SetViewports",VIEWPORT},{"SetDepthRange",DEPTH_RANGE},{"SetDepthRanges",DEPTH_RANGE},
        {"SetStencilRef",STENCIL_REF},{"SetStencilValueMask",STENCIL_READ},{"SetStencilMask",STENCIL_WRITE},{"SetBlendColor",BLEND_COLOR},
        {"SetAlphaRef",ALPHA_REF},{"SetPointSize",POINT_SIZE},{"SetSampleMask",SAMPLE_MASK},{"SetRasterizerDiscard",RASTER_DISCARD},
        {"SetDepthClamp",DEPTH_CLAMP},{"SetPrimitiveRestart",RESTART},{"SetPolygonOffsetClamp",OFFSET},
        {"BindSeparateTexture",TEXTURE},{"BindSeparateTextures",TEXTURE},{"BindSeparateSampler",TEXTURE},{"BindSeparateSamplers",TEXTURE},
        {"SetDepthBounds",UNSUPPORTED},{"SetViewportSwizzles",UNSUPPORTED},
        {"DrawArrays",ARRAYS},{"DrawArraysInstanced",ARRAYS},{"DrawElements",ELEMENTS},{"DrawElementsBaseVertex",ELEMENTS},{"DrawElementsInstanced",ELEMENTS},{"DispatchCompute",DISPATCH}
    };
    for(unsigned i=0;i<sizeof names/sizeof *names;i++)if(!strcmp(name,names[i].name))return names[i].op;
    return 0;
}
int web_gpu_draw_known(const char *name){return operation(name)!=0;}
static void release_range(Range *range){if(range->pool)web_gpu_pool_release(range->pool);memset(range,0,sizeof *range);}
void web_gpu_draw_release(WebGpuDrawRecord *r){if(r){for(unsigned i=0;i<r->range_count;i++)release_range(&RANGES(r)[i]);free(r);}}
void web_gpu_draw_state_release(WebGpuDrawState *s){if(s){for(unsigned i=0;i<32;i++)release_range(&s->vertex[i]);for(unsigned t=0;t<6;t++){for(unsigned i=0;i<16;i++)release_range(&s->uniform[t][i]);for(unsigned i=0;i<32;i++)release_range(&s->storage[t][i]);}free(s);}}
static int read_guest(uint64_t a,void *p,size_t n){return a&&gm_read(web_memory_current(),a,p,n)==GM_OK;}
static int hold_range(Range *r,uint64_t a,uint64_t n){
    if(!a||!n){memset(r,0,sizeof *r);return 1;}
    if(n>0x80000000ULL||a>UINT64_MAX-n)return 0;
    if(!web_gpu_pool_hold_address(a,n,&r->pool,&r->offset))return 0;r->address=a;r->size=n;return 1;
}
WebGpuDrawRecord *web_gpu_draw_record(Ctx *c,const char *name,const char **error) {
    *error="invalid NVN graphics command";unsigned op=operation(name),range_count=0;size_t payload=0;uint64_t count=0;int multiple=name[strlen(name)-1]=='s';
    if(op==ATTR||op==STREAM){count=c->x[1];if(count>32)return NULL;payload=(size_t)count*(op==ATTR?4:8);}
    else if(op==VERTEX||op==UNIFORM||op==STORAGE){count=multiple?c->x[op==VERTEX?2:3]:1;if(count>32)return NULL;range_count=(unsigned)count;payload=(size_t)count*sizeof(Range);}
    else if(op==TEXTURE){count=multiple?c->x[3]:1;if(count>32)return NULL;payload=(size_t)count*8;}
    else if(op==BLEND||op==DEPTH)payload=8;
    else if(op==COLOR||op==MASK||op==POLYGON)payload=4;
    else if(op==MULTISAMPLE)payload=24;
    else if(op==BLEND_COLOR)payload=16;
    else if(op==VIEWPORT||op==DEPTH_RANGE){count=multiple?c->x[2]:1;if(count>16)return NULL;payload=(size_t)count*(op==VIEWPORT?16:8);}
    else if(op==ELEMENTS){range_count=1;payload=sizeof(Range);}
    WebGpuDrawRecord *r=calloc(1,sizeof *r+payload);if(!r){*error="graphics record allocation failed";return NULL;}
    r->op=op;r->range_count=range_count;memcpy(r->x,c->x+1,sizeof r->x);for(unsigned i=0;i<4;i++)r->f[i]=c->v[i].f[0];
    if(r->op==ATTR||r->op==STREAM){
        if(c->x[1]>32)goto failed;r->count=(unsigned)c->x[1];
        if(r->count&&!read_guest(c->x[2],r->data,r->count*(r->op==ATTR?4:8)))goto failed;
    }else if(r->op==VERTEX||r->op==UNIFORM||r->op==STORAGE){
        int vertex=r->op==VERTEX,multiple=name[strlen(name)-1]=='s';unsigned max=r->op==UNIFORM?16:32;
        uint64_t first=c->x[vertex?1:2],count=multiple?c->x[vertex?2:3]:1,source=c->x[vertex?3:4];
        if((!vertex&&c->x[1]>=6)||first>max||count>max-first)goto failed;
        r->first=(unsigned)first;r->count=(unsigned)count;r->stage=vertex?0:(unsigned)c->x[1];
        for(unsigned i=0;i<r->count;i++){
            uint64_t values[2];
            if(multiple){if(source){if(!read_guest(source+(uint64_t)i*16,values,16))goto failed;}else values[0]=values[1]=0;}
            else{values[0]=c->x[vertex?2:3];values[1]=c->x[vertex?3:4];}
            if(!hold_range(&RANGES(r)[i],values[0],values[1])){*error="invalid NVN graphics buffer range";goto failed;}
        }
    }else if(r->op==TEXTURE){
        int multiple=name[strlen(name)-1]=='s';uint64_t first=c->x[2],count=multiple?c->x[3]:1;
        r->kind=strstr(name,"SeparateTexture")?1:strstr(name,"SeparateSampler")?2:0;
        if(c->x[1]>=6||first>32||count>32-first)goto failed;r->stage=(unsigned)c->x[1];r->first=(unsigned)first;r->count=(unsigned)count;
        if(multiple){if(c->x[4]&&!read_guest(c->x[4],r->data,r->count*8))goto failed;}else memcpy(r->data,c->x+3,8);
    }else if(r->op==BLEND||r->op==COLOR||r->op==MASK||r->op==POLYGON||r->op==DEPTH||r->op==MULTISAMPLE||r->op==BLEND_COLOR){
        size_t n=r->op==BLEND||r->op==DEPTH?8:r->op==MULTISAMPLE?24:r->op==BLEND_COLOR?16:4;
        if(!read_guest(c->x[1],r->data,n))goto failed;
    }else if(r->op==VIEWPORT||r->op==DEPTH_RANGE){
        int multiple=name[strlen(name)-1]=='s';r->first=multiple?(unsigned)c->x[1]:0;r->count=multiple?(unsigned)c->x[2]:1;
        if((multiple&&(c->x[1]>16||c->x[2]>16-c->x[1])))goto failed;
        if(multiple){if(r->count&&!read_guest(c->x[3],r->data,r->count*(r->op==VIEWPORT?16:8)))goto failed;}
        else if(r->op==VIEWPORT){float v[4];for(unsigned i=0;i<4;i++)v[i]=(float)(int32_t)c->x[i+1];memcpy(r->data,v,16);}
        else memcpy(r->data,r->f,8);
    }else if(r->op==ARRAYS||r->op==ELEMENTS){
        if(c->x[1]!=4&&c->x[1]!=5){*error="NVN draw topology/index type needs lowering";goto failed;}
        if(r->op==ARRAYS){
            if(!strcmp(name,"DrawArrays")){r->x[3]=0;r->x[4]=1;}
            if(c->x[2]>INT32_MAX||c->x[3]>UINT32_MAX||r->x[3]>UINT32_MAX||r->x[4]>UINT32_MAX)goto failed;
        }else{
            if(c->x[2]>2||c->x[3]>UINT32_MAX){*error="NVN draw topology/index type needs lowering";goto failed;}
            if(!strcmp(name,"DrawElements"))r->x[4]=0;
            if(strcmp(name,"DrawElementsInstanced")){r->x[5]=0;r->x[6]=1;}
            if(r->x[5]>UINT32_MAX||r->x[6]>UINT32_MAX)goto failed;
            if(c->x[3]&&r->x[6]&&!hold_range(&RANGES(r)[0],c->x[4],c->x[3]*(1ULL<<c->x[2]))){*error="invalid NVN index buffer range";goto failed;}
        }
    }else if(r->op==DISPATCH){
        for(unsigned i=0;i<3;i++)if(r->x[i]>UINT32_MAX)goto failed;
    }else if(r->op==UNSUPPORTED){
        if(!strcmp(name,"SetDepthBounds")){r->kind=1;r->x[0]=!!r->x[0];}
        else{r->kind=2;if(c->x[1]>16||c->x[2]>16-c->x[1])goto failed;
            if(c->x[1]==0&&c->x[2]){uint32_t swizzle[4];if(!read_guest(c->x[3],swizzle,16))goto failed;
                r->x[0]=(swizzle[0]!=0||swizzle[1]!=2||swizzle[2]!=4||swizzle[3]!=6);}
            else r->kind=0;
        }
    }
    return r;
failed:web_gpu_draw_release(r);return NULL;
}
static int replace_range(Range *dst,const Range *src){
    if(src->pool&&!web_gpu_pool_hold(src->pool,src->offset,src->size))return 0;
    release_range(dst);*dst=*src;return 1;
}
typedef struct {char *text;size_t used,capacity;const char *error;} Json;
static void append(Json *j,const char *format,...){
    if(j->error)return;va_list args;va_start(args,format);int n=vsnprintf(j->text+j->used,j->capacity-j->used,format,args);va_end(args);
    if(n<0||(size_t)n>=j->capacity-j->used){j->error="graphics packet exceeds staging limit";return;}j->used+=(size_t)n;
}
static void bytes(Json *j,const uint8_t *data,unsigned n){append(j,"[");for(unsigned i=0;i<n;i++)append(j,"%s%u",i?",":"",data[i]);append(j,"]");}
static void floats(Json *j,const float *data,unsigned n){append(j,"[");for(unsigned i=0;i<n;i++){if(!isfinite(data[i])){j->error="non-finite NVN graphics state";return;}append(j,"%s%.9g",i?",":"",(double)data[i]);}append(j,"]");}
static void gpu_range(Json *j,const Range *range,int brackets){
    uint32_t handle=0;uint64_t offset=0;
    if(range->address&&range->size&&!web_gpu_draw_range(range->address,range->size,&handle,&offset)){j->error="graphics buffer staging failed";return;}
    append(j,"%s%u,%llu,%llu%s",brackets?"[":"",handle,(unsigned long long)offset,(unsigned long long)range->size,brackets?"]":"");
}
static void target(Json *j,const WebGpuDrawTarget *t){if(!t->handle){append(j,"null");return;}append(j,"{\"handle\":%u,\"mip\":%u,\"layer\":%u,\"format\":%u,\"layers\":%u}",t->handle,t->mip,t->layer,t->format,t->layers);}
static const char *submit_draw(WebGpuDrawState *s,const WebGpuDrawRecord *r,const WebGpuRenderState *render,const uint32_t programs[6],uint64_t texture_pool,uint64_t sampler_pool){
    unsigned indexed=r->op==ELEMENTS;uint32_t count=(uint32_t)r->x[2],instances=(uint32_t)r->x[indexed?6:4];
    if(!count||!instances)return NULL;
    if(s->unsupported)return "NVN graphics state needs separate descriptor/image/viewport lowering";
    WebGpuDrawTarget targets[9];int32_t scissor[4];uint32_t extent[2];const char *error=web_gpu_render_draw_targets(render,targets,scissor,extent);if(error)return error;
    if(!programs[0]||!programs[1])return "NVN draw requires retained vertex and fragment programs";
    Json j={.text=malloc(131072),.capacity=131072};if(!j.text)return "graphics staging allocation failed";
    uint32_t index_handle=0;uint64_t index_offset=0;
    if(indexed&&!web_gpu_draw_range(RANGES(r)[0].address,RANGES(r)[0].size,&index_handle,&index_offset)){free(j.text);return "NVN index staging failed";}
    append(&j,"{\"version\":1,\"programs\":[%u,%u],\"primitive\":[%u,%d,%u,%d,%u,%u,%u,%llu]",programs[0],programs[1],(uint32_t)r->x[0],indexed?(int)r->x[1]:-1,count,
           (int32_t)r->x[indexed?4:1],(uint32_t)r->x[indexed?5:3],instances,index_handle,(unsigned long long)index_offset);
    append(&j,",\"attributes\":[");for(unsigned i=0;i<32;i++)append(&j,"%s%u",i?",":"",s->attributes[i]);append(&j,"],\"streams\":[");
    for(unsigned i=0;i<32;i++){int used=0;for(unsigned a=0;a<32;a++)if((s->attributes[a]&255)&&((s->attributes[a]>>8)&31)==i)used=1;
        append(&j,"%s[%u,%u,",i?",":"",s->streams[i][0],s->streams[i][1]);if(used)gpu_range(&j,&s->vertex[i],0);else append(&j,"0,0,0");append(&j,"]");}append(&j,"]");
    for(unsigned kind=0;kind<2;kind++){
        append(&j,",\"%s\":[",kind?"ssbo":"ubo");unsigned n=kind?16:15;
        for(unsigned stage=0;stage<2;stage++){append(&j,"%s[",stage?",":"");for(unsigned i=0;i<n;i++){if(i)append(&j,",");gpu_range(&j,kind?&s->storage[stage][i]:&s->uniform[stage][i],1);}append(&j,"]");}append(&j,"]");
    }
    append(&j,",\"textures\":[");
    for(unsigned stage=0;stage<2;stage++){
        append(&j,"%s[",stage?",":"");
        for(unsigned i=0;i<32;i++){
            if(i)append(&j,",");uint64_t handle=s->textures[stage][i];if(!(handle&0x100000000ULL)){append(&j,"null");continue;}
            WebGpuTextureBinding t;WebGpuSamplerParams sampler;
            if(!web_gpu_registered_texture(texture_pool,(uint32_t)handle&0xfffff,&t)||t.image||!web_gpu_registered_sampler(sampler_pool,((uint32_t)handle>>20)&4095,&sampler)){append(&j,"null");continue;}
            append(&j,"{\"texture\":%u,\"view\":%u,\"sampler\":",t.texture,t.view_handle);bytes(&j,(const uint8_t *)&sampler,sizeof sampler);
            append(&j,",\"swizzle\":");bytes(&j,(t.view.set&0x88)==0x88?t.view.swz:t.builder.swz,4);append(&j,"}");
        }append(&j,"]");
    }
    append(&j,"],\"colors\":[");for(unsigned i=0;i<8;i++){if(i)append(&j,",");target(&j,&targets[i]);}append(&j,"],\"depth\":");target(&j,&targets[8]);
    append(&j,",\"scissor\":[%d,%d,%d,%d],\"viewport\":[",scissor[0],scissor[1],scissor[2],scissor[3]);
    float viewport[6]={0,0,(float)extent[0],(float)extent[1],s->depth_range[0],s->depth_range[1]};if(s->has_viewport)memcpy(viewport,s->viewport,16);
    for(unsigned i=0;i<6;i++){if(!isfinite(viewport[i]))j.error="non-finite NVN viewport";append(&j,"%s%.9g",i?",":"",(double)viewport[i]);}
    append(&j,"],\"blend\":[");for(unsigned i=0;i<8;i++){if(i)append(&j,",");bytes(&j,s->blend[i],8);}append(&j,"],\"color\":");bytes(&j,s->color,4);
    append(&j,",\"polygon\":");bytes(&j,s->polygon,4);append(&j,",\"ds\":");bytes(&j,s->depth,8);append(&j,",\"stencil\":[");for(unsigned i=0;i<6;i++)append(&j,"%s%u",i?",":"",s->stencil[i]);
    append(&j,"],\"blendColor\":");floats(&j,s->blend_color,4);
    if(!isfinite(s->alpha_ref)||!isfinite(s->point_size))j.error="non-finite NVN graphics constant";
    append(&j,",\"alphaRef\":%.9g,\"pointSize\":%.9g,\"channelMask\":%u,\"sampleMask\":%u,\"flags\":[%u,%u,%u,%u,%u]}",(double)s->alpha_ref,(double)s->point_size,s->channel_mask,s->sample_mask,
           s->raster_discard,s->depth_clamp,s->restart,(unsigned)(s->offset[0]!=0||s->offset[1]!=0||s->offset[2]!=0),(unsigned)(s->multisample[1]||s->multisample[2]>1||s->multisample[5]));
    if(!j.error){WebGpuReply reply=web_gpu_request(24,0,0,j.used,j.text,j.used);if(reply.status)j.error="WebGPU graphics draw failed";}
    error=j.error;free(j.text);return error;
}
static const char *submit_compute(WebGpuDrawState *s,const WebGpuDrawRecord *r,const uint32_t programs[6],uint64_t texture_pool,uint64_t sampler_pool){
    if(!r->x[0]||!r->x[1]||!r->x[2])return NULL;
    if(!programs[5])return "NVN dispatch requires a retained compute program";
    Json j={.text=malloc(131072),.capacity=131072};if(!j.text)return "compute staging allocation failed";
    append(&j,"{\"version\":1,\"program\":%u,\"groups\":[%u,%u,%u],\"ubo\":[",programs[5],(uint32_t)r->x[0],(uint32_t)r->x[1],(uint32_t)r->x[2]);
    for(unsigned i=0;i<15;i++){if(i)append(&j,",");gpu_range(&j,&s->uniform[5][i],1);}append(&j,"],\"ssbo\":[");
    for(unsigned i=0;i<16;i++){if(i)append(&j,",");gpu_range(&j,&s->storage[5][i],1);}append(&j,"],\"textures\":[");
    for(unsigned i=0;i<32;i++){
        if(i)append(&j,",");uint64_t handle=s->textures[5][i];
        WebGpuTextureBinding t;WebGpuSamplerParams sampler;
        if(!(handle&0x100000000ULL)||!web_gpu_registered_texture(texture_pool,(uint32_t)handle&0xfffff,&t)||t.image||!web_gpu_registered_sampler(sampler_pool,((uint32_t)handle>>20)&4095,&sampler)){append(&j,"null");continue;}
        append(&j,"{\"texture\":%u,\"view\":%u,\"sampler\":",t.texture,t.view_handle);bytes(&j,(const uint8_t *)&sampler,sizeof sampler);append(&j,",\"swizzle\":");bytes(&j,(t.view.set&0x88)==0x88?t.view.swz:t.builder.swz,4);append(&j,"}");
    }
    append(&j,"]}");
    if(!j.error){
        WebGpuReply reply=web_gpu_request(26,0,0,j.used,j.text,j.used);
        if(reply.status)j.error="WebGPU compute dispatch failed";
        else if(reply.value&0xffff0000u)j.error="Invalid compute writeback mask";
        else for(unsigned i=0;i<16;i++)if(reply.value&(1u<<i)){
            const Range *range=&s->storage[5][i];
            if(!range->address||!range->size||!web_gpu_writeback_range(range->address,range->size)){j.error="Compute storage guest writeback failed";break;}
        }
    }
    const char *error=j.error;free(j.text);return error;
}
const char *web_gpu_draw_execute(WebGpuDrawState **state,const WebGpuDrawRecord *r,const WebGpuRenderState *render,const uint32_t programs[6],uint64_t texture_pool,uint64_t sampler_pool){
    WebGpuDrawState *s=*state;if(!s){
        s=calloc(1,sizeof *s);if(!s)return "graphics queue state allocation failed";*state=s;
        s->channel_mask=s->sample_mask=UINT32_MAX;s->depth_range[1]=s->point_size=1;
        for(unsigned i=2;i<6;i++)s->stencil[i]=255;
        memcpy(s->color,(uint8_t[]){0,3,8,0},4);memcpy(s->polygon,(uint8_t[]){0,1,2,0},4);uint64_t ds=0x1111118822ULL;memcpy(s->depth,&ds,8);
        s->multisample[0]=s->multisample[2]=1;
        for(unsigned i=0;i<8;i++)memcpy(s->blend[i],(uint8_t[]){(uint8_t)i,2,1,2,1,1,1,0},8);
    }
    if(r->op==ATTR){memset(s->attributes,0,sizeof s->attributes);memcpy(s->attributes,r->data,r->count*4);}
    else if(r->op==STREAM)memcpy(s->streams,r->data,r->count*8);
    else if(r->op==VERTEX||r->op==UNIFORM||r->op==STORAGE){
        Range *ranges=r->op==VERTEX?s->vertex:r->op==UNIFORM?s->uniform[r->stage]:s->storage[r->stage];
        for(unsigned i=0;i<r->count;i++)if(!replace_range(&ranges[r->first+i],&RANGES(r)[i]))return "NVN graphics pool retention failed";
    }else if(r->op==TEXTURE){for(unsigned i=0;i<r->count;i++){uint64_t value;memcpy(&value,r->data+i*8,8);uint64_t *dst=&s->textures[r->stage][r->first+i];
        if(r->kind==1)*dst=(*dst&0xfff00000ULL)|(value&0x1000fffffULL);else if(r->kind==2)*dst=(*dst&0x1000fffffULL)|(value&0xfff00000ULL);else *dst=value;}}
    else if(r->op==BLEND)memcpy(s->blend[r->data[0]&7],r->data,8);
    else if(r->op==COLOR)memcpy(s->color,r->data,4);
    else if(r->op==MASK)memcpy(&s->channel_mask,r->data,4);
    else if(r->op==POLYGON)memcpy(s->polygon,r->data,4);
    else if(r->op==DEPTH)memcpy(s->depth,r->data,8);
    else if(r->op==MULTISAMPLE)memcpy(s->multisample,r->data,24);
    else if(r->op==VIEWPORT){if(r->first==0&&r->count){memcpy(s->viewport,r->data,16);s->has_viewport=1;}}
    else if(r->op==DEPTH_RANGE){if(r->first==0&&r->count)memcpy(s->depth_range,r->data,8);}
    else if(r->op>=STENCIL_REF&&r->op<=STENCIL_WRITE){unsigned first=(r->op-STENCIL_REF)*2;for(unsigned i=0;i<2;i++)if(r->x[0]&(1u<<i))s->stencil[first+i]=(uint32_t)r->x[1];}
    else if(r->op==BLEND_COLOR)memcpy(s->blend_color,r->data,16);
    else if(r->op==ALPHA_REF)s->alpha_ref=r->f[0];
    else if(r->op==POINT_SIZE)s->point_size=r->f[0];
    else if(r->op==SAMPLE_MASK)s->sample_mask=(uint32_t)r->x[0];
    else if(r->op==RASTER_DISCARD)s->raster_discard=!!r->x[0];
    else if(r->op==DEPTH_CLAMP)s->depth_clamp=!!r->x[0];
    else if(r->op==RESTART)s->restart=!!r->x[0];
    else if(r->op==OFFSET)memcpy(s->offset,r->f,12);
    else if(r->op==UNSUPPORTED){if(r->kind){unsigned bit=1u<<(r->kind-1);s->unsupported=(s->unsupported&~bit)|(r->x[0]?bit:0);}}
    else if(r->op==ARRAYS||r->op==ELEMENTS)return submit_draw(s,r,render,programs,texture_pool,sampler_pool);
    else if(r->op==DISPATCH)return submit_compute(s,r,programs,texture_pool,sampler_pool);
    return NULL;
}
