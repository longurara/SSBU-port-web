/* Texture storage and uploads share the native backend's portable Tegra layout
 * library. No game pointer is passed to that library or to WebGPU. */
#include "host_gpu_texture.h"
#include "host_gpu_bridge.h"
#include "tex_decode.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
typedef struct WebGpuTextureRef {uint64_t key,storage;uint32_t gpu;atomic_uint references;WebGpuTextureBuilder b;struct WebGpuTextureRef *next;} Texture;
static Texture *textures;
static pthread_mutex_t texture_lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t format_once=PTHREAD_ONCE_INIT;
static void init_formats(void){(void)tex_format(1);}
static void fail(const char *what,uint64_t address){pthread_mutex_unlock(&texture_lock);guest_trap(what,address);}
static Texture *find(uint64_t key){for(Texture *t=textures;t;t=t->next)if(t->key==key)return t;return NULL;}
int web_gpu_texture_handle(uint64_t object,uint32_t *handle) {
    pthread_mutex_lock(&texture_lock);Texture *t=find(object);if(t)*handle=t->gpu;
    pthread_mutex_unlock(&texture_lock);return t!=NULL;
}
WebGpuTextureRef *web_gpu_texture_hold(uint64_t object) {
    pthread_mutex_lock(&texture_lock);Texture *t=find(object);
    if(t&&atomic_load(&t->references)!=UINT32_MAX)atomic_fetch_add(&t->references,1);else t=NULL;
    pthread_mutex_unlock(&texture_lock);return t;
}
void web_gpu_texture_release(WebGpuTextureRef *t) {
    if(!t||!atomic_fetch_sub(&t->references,1))guest_trap("invalid texture reference release",0);
}
void web_gpu_texture_snapshot(const WebGpuTextureRef *t,WebGpuTextureBuilder *builder,uint32_t *handle){*builder=t->b;*handle=t->gpu;}
static uint8_t *upload_packet(const WebGpuTextureBuilder *b,const TexLayout *layout,uint64_t storage,size_t *length) {
    if(layout->data_size>storage||layout->data_size>128*1024*1024)return NULL;
    size_t total=36;
    for(int layer=0;layer<layout->layers;layer++)for(int mip=0;mip<layout->levels;mip++) {
        size_t size=layout->lvl_linear_size[mip];
        if(size>128*1024*1024-24||total>128*1024*1024-24-size)return NULL;
        total+=24+size;
    }
    uint8_t *source=malloc(layout->data_size),*packet=malloc(total);
    if(!source||!packet){free(source);free(packet);return NULL;}
    /* Like the native frontend, texels come from the memory pool. The packaged
     * data/layout pointers remain metadata and are not substituted for storage. */
    int copied=web_gpu_pool_copy(b->pool,b->offset,source,layout->data_size);
    if(!copied){free(source);free(packet);return NULL;}
    const uint32_t header[]={0x5854564e,(uint32_t)b->format,(uint32_t)b->target,(uint32_t)b->width,(uint32_t)b->height,
        (uint32_t)b->depth,(uint32_t)layout->levels,(uint32_t)b->samples,(uint32_t)(layout->layers*layout->levels)};
    memcpy(packet,header,sizeof header);size_t offset=sizeof header;
    for(int layer=0;layer<layout->layers;layer++)for(int mip=0;mip<layout->levels;mip++) {
        uint32_t fields[]={(uint32_t)mip,(uint32_t)layer,(uint32_t)layout->lvl_w[mip],(uint32_t)layout->lvl_h[mip],
            (uint32_t)layout->lvl_d[mip],(uint32_t)layout->lvl_linear_size[mip]};
        memcpy(packet+offset,fields,sizeof fields);offset+=sizeof fields;
        tex_deswizzle_level(layout,mip,tex_level_ptr(layout,source,mip,layer),packet+offset);
        offset+=layout->lvl_linear_size[mip];
    }
    free(source);*length=total;return packet;
}
int web_gpu_texture_object_import(Ctx *c,const char *name) {
    if(strncmp(name,"nvnTexture",10)||!strncmp(name,"nvnTextureBuilder",17)||!strncmp(name,"nvnTexturePool",14)||!strncmp(name,"nvnTextureView",14))return 0;
    const char *operation=name+10;
    const char *known[]={"Initialize","Finalize","GetFlags","GetTarget","GetFormat","GetWidth","GetHeight","GetDepth","GetLevels","GetSamples",
      "GetStride","GetMemoryPool","GetMemoryOffset","GetTextureAddress","GetStorageSize","GetDepthStencilMode","GetSwizzle","GetDebugID","SetDebugLabel","Compare",
      "GetZCullStorageSize","GetStorageClass","GetRawStorageClass","GetSparseTileLayout","GetViewOffset","FlushTexels","WriteTexels","WriteTexelsStrided"};
    int recognised=0;for(unsigned i=0;i<sizeof known/sizeof *known;i++)if(!strcmp(operation,known[i]))recognised=1;
    if(!recognised)return 0;
    pthread_mutex_lock(&texture_lock);Texture *t=find(c->x[0]);
    if(!strcmp(operation,"Initialize")) {
        WebGpuTextureBuilder b;web_read(c->x[1],&b,sizeof b);
        if(t||!c->x[0]||rd64(c->x[1]+96)!=web_gpu_device_address()||b.width<=0||b.height<=0||b.depth<=0||b.width>32768||b.height>32768||b.depth>2048||
           b.levels<1||b.levels>TEX_MAX_LEVELS||b.target<0||b.target>10||!b.pool){c->x[0]=0;pthread_mutex_unlock(&texture_lock);return 1;}
        if(b.samples<0||b.stride<0){c->x[0]=0;pthread_mutex_unlock(&texture_lock);return 1;}
        if(b.glname)fail("OpenGL texture interop is unavailable on WebGPU",b.glname);
        if(b.flags&0x20)fail("sparse texture tile mapping not ported",c->x[1]);
        if(b.flags&0x40)fail("linear render-target texture layout needs source validation",c->x[1]);
        if(b.samples>1||b.target==5||b.target==6||b.target==10)fail("multisample/buffer texture storage not ported",c->x[1]);
        pthread_once(&format_once,init_formats);
        const TexFormat *format=tex_format(b.format);
        if(!format)fail("unknown NVN texture format",b.format);
        if(format->kind&&b.target!=1&&b.target!=3&&b.target!=4&&b.target!=7&&b.target!=8&&b.target!=9)fail("depth/stencil texture dimension not ported",b.format);
        uint64_t row_bytes=((uint64_t)b.width+format->bw-1)/format->bw*format->bpb;
        if((b.flags&0x50)&&b.stride&&(b.stride<(int64_t)row_bytes||(uint64_t)b.stride>SIZE_MAX)) {
            c->x[0]=0;pthread_mutex_unlock(&texture_lock);return 1;
        }
        TexLayout layout;
        if(tex_layout(&layout,b.target,b.format,b.width,b.height,b.depth,b.levels,b.flags,(size_t)b.stride,b.has_layout&&b.layout[0]<=4?b.layout[0]:-1)!=0||layout.levels!=b.levels)
            fail("invalid NVN texture layout",c->x[1]);
        uint64_t storage=web_nvn_layout_query(c,&b,0x428760,NULL);
        if(!storage||storage>128*1024*1024||!web_gpu_pool_hold(b.pool,b.offset,storage)){c->x[0]=0;pthread_mutex_unlock(&texture_lock);return 1;}
        size_t length=0;uint8_t *packet=upload_packet(&b,&layout,storage,&length);
        if(!packet){web_gpu_pool_release(b.pool);fail("texture staging/layout exceeds source pool storage",c->x[1]);}
        WebGpuReply reply=web_gpu_request(14,0,0,length,packet,length);free(packet);
        if(reply.status){web_gpu_pool_release(b.pool);fail("WebGPU texture creation/upload failed",c->x[1]);}
        t=calloc(1,sizeof *t);
        if(!t){web_gpu_request(15,reply.value,0,0,NULL,0);web_gpu_pool_release(b.pool);fail("texture metadata allocation failed",c->x[0]);}
        atomic_init(&t->references,0);t->key=c->x[0];t->b=b;t->storage=storage;t->gpu=reply.value;t->next=textures;textures=t;c->x[0]=1;
    } else {
        if(!t)fail("invalid NVN texture object",c->x[0]);
        if(!strcmp(operation,"Finalize")) {
            if(atomic_load(&t->references))fail("texture still referenced by an active submission",t->key);
            if(web_gpu_request(15,t->gpu,0,0,NULL,0).status)fail("GPU texture release failed",t->key);
            web_gpu_pool_release(t->b.pool);Texture **link=&textures;while(*link!=t)link=&(*link)->next;*link=t->next;free(t);
        } else if(!strcmp(operation,"GetFlags"))c->x[0]=(uint32_t)t->b.flags;
        else if(!strcmp(operation,"GetTarget"))c->x[0]=(uint32_t)t->b.target;
        else if(!strcmp(operation,"GetFormat"))c->x[0]=(uint32_t)t->b.format;
        else if(!strcmp(operation,"GetWidth"))c->x[0]=(uint32_t)t->b.width;
        else if(!strcmp(operation,"GetHeight"))c->x[0]=(uint32_t)t->b.height;
        else if(!strcmp(operation,"GetDepth"))c->x[0]=(uint32_t)t->b.depth;
        else if(!strcmp(operation,"GetLevels"))c->x[0]=(uint32_t)t->b.levels;
        else if(!strcmp(operation,"GetSamples"))c->x[0]=(uint32_t)t->b.samples;
        else if(!strcmp(operation,"GetDepthStencilMode"))c->x[0]=(uint32_t)t->b.dsmode;
        else if(!strcmp(operation,"GetStride"))c->x[0]=(uint64_t)t->b.stride;
        else if(!strcmp(operation,"GetMemoryPool"))c->x[0]=t->b.pool;
        else if(!strcmp(operation,"GetMemoryOffset"))c->x[0]=t->b.offset;
        else if(!strcmp(operation,"GetTextureAddress"))c->x[0]=web_gpu_pool_address(t->b.pool,t->b.offset);
        else if(!strcmp(operation,"GetStorageSize"))c->x[0]=(uint32_t)t->storage;
        else if(!strcmp(operation,"GetZCullStorageSize"))c->x[0]=0;
        else if(!strcmp(operation,"GetStorageClass"))c->x[0]=web_nvn_layout_query(c,&t->b,0x427a10,NULL);
        else if(!strcmp(operation,"GetRawStorageClass"))c->x[0]=web_nvn_layout_query(c,&t->b,0x427b30,NULL);
        else if(!strcmp(operation,"GetSparseTileLayout")) {
            uint32_t output[4]={0};web_nvn_layout_query(c,&t->b,0x4290b0,output);
            if(c->x[1])web_write(c->x[1],output,16);
        } else if(!strcmp(operation,"GetViewOffset")) {
            WebGpuTextureView view;TexLayout layout;
            if(!c->x[1]||(t->b.flags&0x10))c->x[0]=0;
            else {
                web_read(c->x[1],&view,sizeof view);pthread_once(&format_once,init_formats);
                if(tex_layout(&layout,t->b.target,t->b.format,t->b.width,t->b.height,t->b.depth,t->b.levels,t->b.flags,(size_t)t->b.stride,t->b.has_layout&&t->b.layout[0]<=4?t->b.layout[0]:-1)!=0)c->x[0]=0;
                else {
                    int level=(view.set&0x81)==0x81?view.lvl0:0,layer=(view.set&0x82)==0x82?view.lay0:0;
                    if(level<0||level>=layout.levels||layer<0||layer>=layout.layers)c->x[0]=0;
                    else c->x[0]=(uint64_t)(tex_level_ptr(&layout,(const uint8_t *)0x1000,level,layer)-(const uint8_t *)0x1000);
                }
            }
        } else if(!strcmp(operation,"GetDebugID"))c->x[0]=t->key;
        else if(!strcmp(operation,"GetSwizzle")){for(unsigned i=0;i<4;i++)if(c->x[i+1])wr32(c->x[i+1],t->b.swz[i]);}
        else if(!strcmp(operation,"Compare")){Texture *other=find(c->x[1]);c->x[0]=other&&!memcmp(&t->b,&other->b,sizeof t->b);}
        else if(!strcmp(operation,"WriteTexels")||!strcmp(operation,"WriteTexelsStrided")) {
            WebGpuTextureView view={0};int32_t region[6];
            if((c->x[1]&&gm_read(web_memory_current(),c->x[1],&view,sizeof view)!=GM_OK)||!c->x[2]||!c->x[3]||gm_read(web_memory_current(),c->x[2],region,sizeof region)!=GM_OK)
                fail("invalid WriteTexels arguments",c->x[0]);
            if(((view.set&0x84)==0x84&&view.format!=t->b.format)||((view.set&0xa0)==0xa0&&view.target!=t->b.target))
                fail("WriteTexels view format/target reinterpretation not ported",c->x[0]);
            int mip=(view.set&0x81)==0x81?view.lvl0:0,layer=(view.set&0x82)==0x82?view.lay0:0;
            if(mip<0||mip>=t->b.levels||layer<0)fail("invalid WriteTexels mip/layer",c->x[0]);
            for(unsigned i=0;i<6;i++)if(region[i]<0)fail("negative WriteTexels region",c->x[0]);
            uint32_t width=(uint32_t)t->b.width>>mip,height=(uint32_t)t->b.height>>mip;
            if(!width)width=1;if(!height)height=1;
            int is3d=t->b.target==2,array=t->b.target==3||t->b.target==4||t->b.target==8||t->b.target==9;
            uint32_t layers=t->b.target==8?6:array?(uint32_t)t->b.depth:1;
            uint32_t limit=is3d?((uint32_t)t->b.depth>>mip):layers;if(!limit)limit=1;
            if((is3d||!array)&&layer)fail("invalid WriteTexels layer for non-array texture",c->x[0]);
            uint64_t z=(uint64_t)(uint32_t)region[2]+(array?(unsigned)layer:0),depth=region[5]?(uint32_t)region[5]:1;
            if((uint32_t)region[0]>width||(uint32_t)region[3]>width-(uint32_t)region[0]||
               (uint32_t)region[1]>height||(uint32_t)region[4]>height-(uint32_t)region[1]||z>limit||depth>limit-z)
                fail("WriteTexels region exceeds texture bounds",c->x[0]);
            pthread_once(&format_once,init_formats);
            const TexFormat *format=tex_format(t->b.format);if(!format)fail("unknown WriteTexels format",t->b.format);
            if(region[0]%format->bw||region[1]%format->bh||
               (region[3]%format->bw&&(uint32_t)(region[0]+region[3])!=width)||(region[4]%format->bh&&(uint32_t)(region[1]+region[4])!=height))
                fail("unaligned WriteTexels block region",c->x[0]);
            uint32_t w=(uint32_t)region[3],h=(uint32_t)region[4],d=(uint32_t)depth;
            uint32_t row_bytes=((w+format->bw-1)/format->bw)*format->bpb,rows=(h+format->bh-1)/format->bh;
            if(row_bytes&&rows) {
                int strided=!strcmp(operation,"WriteTexelsStrided");
                uint64_t row_stride=(strided&&c->x[4])?c->x[4]:row_bytes;
                if(row_stride<row_bytes||row_stride>128*1024*1024)fail("invalid WriteTexels row stride",c->x[0]);
                uint64_t min_image=(uint64_t)(rows-1)*row_stride+row_bytes;
                uint64_t image_stride=(strided&&c->x[5])?c->x[5]:row_stride*rows;
                if(image_stride<min_image||image_stride>128*1024*1024)fail("invalid WriteTexels image stride",c->x[0]);
                uint64_t tight=(uint64_t)row_bytes*rows*d;
                if(tight>128*1024*1024-40)fail("WriteTexels payload too large",c->x[0]);
                size_t packet_len=40+(size_t)tight;
                uint8_t *packet=malloc(packet_len);if(!packet)fail("WriteTexels allocation failed",c->x[0]);
                uint32_t header[]={1u,t->gpu,(uint32_t)mip,(uint32_t)region[0],(uint32_t)region[1],(uint32_t)z,w,h,d,(uint32_t)tight};
                memcpy(packet,header,sizeof header);
                uint8_t *dst=packet+sizeof header;
                for(uint32_t zi=0;zi<d;zi++)for(uint32_t yi=0;yi<rows;yi++) {
                    uint64_t src_addr=c->x[3]+(uint64_t)zi*image_stride+(uint64_t)yi*row_stride;
                    if(gm_read(web_memory_current(),src_addr,dst,row_bytes)!=GM_OK){free(packet);fail("WriteTexels source read out of bounds",src_addr);}
                    dst+=row_bytes;
                }
                atomic_fetch_add(&t->references,1);
                pthread_mutex_unlock(&texture_lock);
                WebGpuReply reply=web_gpu_request(17,0,0,packet_len,packet,packet_len);
                free(packet);
                web_gpu_texture_release(t);
                if(reply.status)guest_trap("WebGPU WriteTexels upload failed",c->x[0]);
                return 1;
            }
        }
        /* SetDebugLabel and FlushTexels are diagnostic/cache hints. */
    }
    pthread_mutex_unlock(&texture_lock);return 1;
}
