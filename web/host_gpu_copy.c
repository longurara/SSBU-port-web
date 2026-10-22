/* Copy metadata is recorded now; source bytes are read when submitted. This
 * preserves later CPU writes and captures strides separately for each command. */
#include "host_gpu_copy.h"
#include "host_gpu_texture.h"
#include "host_gpu_bridge.h"
#include "tex_decode.h"
#include <stdlib.h>
#include <string.h>
struct WebGpuCopy {
    uint64_t texture_key,source_address,row_stride,image_stride,source_size;
    uint32_t gpu,mip,x,y,z,width,height,depth,rows,row_bytes,tight_size;
};
struct WebGpuCopyLease {WebGpuTextureRef *texture;uint64_t pool;};
void web_gpu_copy_release(WebGpuCopy *copy){free(copy);}
void web_gpu_copy_release_lease(WebGpuCopyLease *lease) {
    if(!lease)return;if(lease->pool)web_gpu_pool_release(lease->pool);
    if(lease->texture)web_gpu_texture_release(lease->texture);free(lease);
}
WebGpuCopy *web_gpu_copy_record(Ctx *c,uint64_t row_stride,uint64_t image_stride,const char **error) {
    *error="invalid buffer-to-texture copy";
    WebGpuCopy *copy=calloc(1,sizeof *copy);if(!copy){*error="copy record allocation failed";return NULL;}
    WebGpuTextureRef *texture=web_gpu_texture_hold(c->x[2]);if(!texture)goto failed;
    WebGpuTextureBuilder b;web_gpu_texture_snapshot(texture,&b,&copy->gpu);web_gpu_texture_release(texture);
    copy->texture_key=c->x[2];copy->source_address=c->x[1];
    WebGpuTextureView view={0};int32_t region[6];
    if((c->x[3]&&gm_read(web_memory_current(),c->x[3],&view,sizeof view)!=GM_OK)||!c->x[4]||gm_read(web_memory_current(),c->x[4],region,sizeof region)!=GM_OK)goto failed;
    if(c->x[5]){*error="buffer-to-texture copy flags not ported";goto failed;}
    if(((view.set&0x84)==0x84&&view.format!=b.format)||((view.set&0xa0)==0xa0&&view.target!=b.target)) {
        *error="copy view format/target reinterpretation not ported";goto failed;
    }
    int mip=(view.set&0x81)==0x81?view.lvl0:0,layer=(view.set&0x82)==0x82?view.lay0:0;
    if(mip<0||mip>=b.levels||layer<0)goto failed;
    for(unsigned i=0;i<6;i++)if(region[i]<0)goto failed;
    uint32_t width=(uint32_t)b.width>>mip,height=(uint32_t)b.height>>mip;
    if(!width)width=1;if(!height)height=1;
    int is3d=b.target==2,array=b.target==3||b.target==4||b.target==8||b.target==9;
    uint32_t layers=b.target==8?6:array?(uint32_t)b.depth:1;
    uint32_t limit=is3d?((uint32_t)b.depth>>mip):layers;if(!limit)limit=1;
    if((is3d||!array)&&layer)goto failed;
    uint64_t z=(uint64_t)(uint32_t)region[2]+(array?(unsigned)layer:0),depth=region[5]?(uint32_t)region[5]:1;
    if((uint32_t)region[0]>width||(uint32_t)region[3]>width-(uint32_t)region[0]||
       (uint32_t)region[1]>height||(uint32_t)region[4]>height-(uint32_t)region[1]||z>limit||depth>limit-z)goto failed;
    if((view.set&0x82)==0x82&&view.nlay>0&&array&&(uint64_t)(uint32_t)region[2]+depth>(unsigned)view.nlay)goto failed;
    const TexFormat *format=tex_format(b.format);if(!format)goto failed;
    if(region[0]%format->bw||region[1]%format->bh||
       (region[3]%format->bw&&(uint32_t)(region[0]+region[3])!=width)||(region[4]%format->bh&&(uint32_t)(region[1]+region[4])!=height))goto failed;
    copy->mip=(uint32_t)mip;copy->x=(uint32_t)region[0];copy->y=(uint32_t)region[1];copy->z=(uint32_t)z;
    copy->width=(uint32_t)region[3];copy->height=(uint32_t)region[4];copy->depth=(uint32_t)depth;
    copy->row_bytes=((copy->width+format->bw-1)/format->bw)*format->bpb;copy->rows=(copy->height+format->bh-1)/format->bh;
    if(!copy->row_bytes||!copy->rows){copy->tight_size=0;return copy;}
    copy->row_stride=row_stride?row_stride:copy->row_bytes;
    if(copy->row_stride<copy->row_bytes||copy->row_stride>128*1024*1024)goto failed;
    uint64_t minimum_image=(uint64_t)(copy->rows-1)*copy->row_stride+copy->row_bytes;
    copy->image_stride=image_stride?image_stride:copy->row_stride*copy->rows;
    if(copy->image_stride<minimum_image||copy->image_stride>128*1024*1024)goto failed;
    copy->source_size=(depth-1)*copy->image_stride+minimum_image;
    uint64_t tight=(uint64_t)copy->row_bytes*copy->rows*depth;
    uint64_t pool,offset;
    if(copy->source_size>128*1024*1024||tight>128*1024*1024-36||!web_gpu_pool_hold_address(c->x[1],copy->source_size,&pool,&offset))goto failed;
    web_gpu_pool_release(pool);
    copy->tight_size=(uint32_t)tight;return copy;
failed:web_gpu_copy_release(copy);return NULL;
}
size_t web_gpu_copy_packet_size(const WebGpuCopy *copy){return copy->tight_size?36+copy->tight_size:0;}
WebGpuCopyLease *web_gpu_copy_prepare(const WebGpuCopy *copy,uint8_t *output) {
    WebGpuCopyLease *lease=calloc(1,sizeof *lease);if(!lease)return NULL;
    lease->texture=web_gpu_texture_hold(copy->texture_key);
    if(!lease->texture){free(lease);return NULL;}
    WebGpuTextureBuilder b;uint32_t gpu;web_gpu_texture_snapshot(lease->texture,&b,&gpu);
    uint64_t offset;
    if(gpu!=copy->gpu||!web_gpu_pool_hold_address(copy->source_address,copy->source_size,&lease->pool,&offset)) {
        web_gpu_copy_release_lease(lease);return NULL;
    }
    uint8_t *source=malloc((size_t)copy->source_size);
    if(!source||!web_gpu_pool_copy(lease->pool,offset,source,(size_t)copy->source_size)){free(source);web_gpu_copy_release_lease(lease);return NULL;}
    uint32_t header[]={copy->gpu,copy->mip,copy->x,copy->y,copy->z,copy->width,copy->height,copy->depth,copy->tight_size};
    memcpy(output,header,sizeof header);output+=sizeof header;
    for(unsigned z=0;z<copy->depth;z++)for(unsigned y=0;y<copy->rows;y++) {
        memcpy(output,source+z*copy->image_stride+y*copy->row_stride,copy->row_bytes);output+=copy->row_bytes;
    }
    free(source);return lease;
}
