/* NVN pool/buffer metadata and real WebGPU storage. Builder fields live in
 * guest memory so copying an opaque builder preserves its configuration. */
#include "runtime.h"
#include "host_gpu_bridge.h"
#include "host_gpu_sampler.h"
#include "host_gpu_texture.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

enum {B_DEVICE=0,B_STORAGE=8,B_SIZE=16,B_OFFSET=24,B_FLAGS=32};
enum {POOL=1,BUFFER=2,TEXTURE_POOL=3,SAMPLER_POOL=4};
typedef struct SamplerSlot {unsigned index;WebGpuSamplerParams value;struct SamplerSlot *next;} SamplerSlot;
typedef struct TextureSlot {unsigned index;WebGpuTextureBinding value;struct TextureSlot *next;} TextureSlot;
typedef struct Resource {
    uint64_t key,memory,size,pool,offset,address;
    uint32_t kind,flags,gpu,references;
    SamplerSlot *samplers;
    TextureSlot **textures;
    struct Resource *next;
} Resource;
static Resource *resources[4096];
static pthread_mutex_t resource_lock=PTHREAD_MUTEX_INITIALIZER;
static uint64_t gpu_only_address=0x7b00000000ULL;
static char last_initialization_error[96];

static unsigned bucket(uint64_t key){return (unsigned)((key>>4)*2654435761u)&4095;}
static Resource *find(uint64_t key,unsigned kind) {
    for(Resource *r=resources[bucket(key)];r;r=r->next)if(r->key==key&&r->kind==kind)return r;
    return NULL;
}
static Resource *create(uint64_t key,unsigned kind) {
    if(!key)return NULL;
    for(Resource *existing=resources[bucket(key)];existing;existing=existing->next)if(existing->key==key)return NULL;
    Resource *r=calloc(1,sizeof *r);if(!r)return NULL;
    r->key=key;r->kind=kind;r->next=resources[bucket(key)];resources[bucket(key)]=r;return r;
}
static void drop(Resource *r) {
    Resource **link=&resources[bucket(r->key)];while(*link!=r)link=&(*link)->next;
    *link=r->next;free(r);
}
static void fail(const char *message,uint64_t address) {
    pthread_mutex_unlock(&resource_lock);guest_trap(message,address);
}
static int mapped(uint64_t address,uint64_t size) {
    if(!address||size>SIZE_MAX||address>UINT64_MAX-size)return 0;
    uint8_t byte;
    for(uint64_t offset=0;offset<size;offset+=4096)
        if(gm_read(web_memory_current(),address+offset,&byte,1)!=GM_OK)return 0;
    return !size||gm_read(web_memory_current(),address+size-1,&byte,1)==GM_OK;
}
static int transfer(Resource *pool,uint64_t offset,uint64_t size,int readback) {
    if(offset>pool->size||size>pool->size-offset)return 0;
    if(!size||!pool->gpu)return 1;
    if(!pool->memory)return 0;
    uint64_t begin=offset&~3ULL,end=(offset+size+3)&~3ULL;
    if(end>pool->size)return 0;
    size_t capacity=(size_t)(end-begin);if(capacity>1048576)capacity=1048576;
    uint8_t *staging=malloc(capacity);if(!staging)return 0;
    int ok=1;
    for(uint64_t position=begin;position<end;) {
        size_t step=(size_t)(end-position);if(step>capacity)step=capacity;
        if(!readback&&gm_read(web_memory_current(),pool->memory+position,staging,step)!=GM_OK){ok=0;break;}
        WebGpuReply reply=web_gpu_request(readback?3:2,pool->gpu,position,step,staging,step);
        if(reply.status){ok=0;break;}
        if(readback&&gm_write(web_memory_current(),pool->memory+position,staging,step)!=GM_OK){ok=0;break;}
        position+=step;
    }
    free(staging);return ok;
}
static int ensure_gpu(Resource *pool) {
    if(pool->gpu)return 1;
    WebGpuReply reply=web_gpu_request(1,0,0,pool->size,NULL,0);
    if(reply.status)return 0;
    pool->gpu=reply.value;
    if(pool->memory&&!transfer(pool,0,pool->size,0)) {
        web_gpu_request(4,pool->gpu,0,0,NULL,0);pool->gpu=0;return 0;
    }
    return 1;
}
int web_gpu_pool_hold(uint64_t key,uint64_t offset,uint64_t size) {
    pthread_mutex_lock(&resource_lock);Resource *pool=find(key,POOL);
    int ok=pool&&offset<=pool->size&&size<=pool->size-offset;
    if(ok)pool->references++;
    pthread_mutex_unlock(&resource_lock);return ok;
}
void web_gpu_pool_release(uint64_t key) {
    pthread_mutex_lock(&resource_lock);Resource *pool=find(key,POOL);
    if(!pool||!pool->references)fail("invalid GPU pool reference",key);
    pool->references--;pthread_mutex_unlock(&resource_lock);
}
int web_gpu_pool_copy(uint64_t key,uint64_t offset,void *output,size_t size) {
    pthread_mutex_lock(&resource_lock);Resource *pool=find(key,POOL);
    int ok=pool&&offset<=pool->size&&size<=pool->size-offset;
    if(ok&&pool->memory&&!(pool->flags&1))ok=gm_read(web_memory_current(),pool->memory+offset,output,size)==GM_OK;
    else if(ok&&size) {
        uint64_t start=offset&~3ULL,end=(offset+size+3)&~3ULL;
        ok=end<=pool->size&&ensure_gpu(pool);
        uint8_t *bytes=ok?malloc((size_t)(end-start)):NULL;
        if(!bytes)ok=0;
        if(ok){WebGpuReply reply=web_gpu_request(3,pool->gpu,start,end-start,bytes,(size_t)(end-start));ok=!reply.status;}
        if(ok)memcpy(output,bytes+offset-start,size);free(bytes);
    }
    pthread_mutex_unlock(&resource_lock);return ok;
}
uint64_t web_gpu_pool_address(uint64_t key,uint64_t offset) {
    pthread_mutex_lock(&resource_lock);Resource *pool=find(key,POOL);
    uint64_t address=pool&&offset<=pool->size?pool->address+offset:0;
    pthread_mutex_unlock(&resource_lock);return address;
}
int web_gpu_pool_hold_address(uint64_t address,uint64_t size,uint64_t *key,uint64_t *offset) {
    pthread_mutex_lock(&resource_lock);Resource *found=NULL;
    for(unsigned i=0;i<4096&&!found;i++)for(Resource *pool=resources[i];pool;pool=pool->next)
        if(pool->kind==POOL&&pool->references!=UINT32_MAX&&address>=pool->address&&address-pool->address<=pool->size&&size<=pool->size-(address-pool->address)){found=pool;break;}
    if(found){found->references++;*key=found->key;*offset=address-found->address;}
    pthread_mutex_unlock(&resource_lock);return found!=NULL;
}

int web_gpu_resolve(uint64_t address,uint64_t size,uint32_t *handle,uint64_t *offset) {
    pthread_mutex_lock(&resource_lock);Resource *found=NULL;
    for(unsigned i=0;i<4096&&!found;i++)for(Resource *r=resources[i];r;r=r->next)
        if(r->kind==POOL&&address>=r->address&&address-r->address<=r->size&&size<=r->size-(address-r->address)){found=r;break;}
    int ok=found&&ensure_gpu(found);
    if(ok){*handle=found->gpu;*offset=address-found->address;}
    pthread_mutex_unlock(&resource_lock);return ok;
}
int web_gpu_registered_sampler(uint64_t key,unsigned index,WebGpuSamplerParams *output) {
    pthread_mutex_lock(&resource_lock);Resource *pool=find(key,SAMPLER_POOL);SamplerSlot *slot=pool?pool->samplers:NULL;
    while(slot&&slot->index!=index)slot=slot->next;
    int found=slot!=NULL;if(found)*output=slot->value;
    pthread_mutex_unlock(&resource_lock);return found;
}
int web_gpu_draw_range(uint64_t address,uint64_t size,uint32_t *handle,uint64_t *offset) {
    pthread_mutex_lock(&resource_lock);Resource *found=NULL;
    for(unsigned i=0;i<4096&&!found;i++)for(Resource *r=resources[i];r;r=r->next)
        if(r->kind==POOL&&address>=r->address&&address-r->address<=r->size&&size<=r->size-(address-r->address)){found=r;break;}
    int ok=found&&ensure_gpu(found);
    if(ok&&found->memory&&!(found->flags&1))ok=transfer(found,address-found->address,size,0);
    if(ok){*handle=found->gpu;*offset=address-found->address;}
    pthread_mutex_unlock(&resource_lock);return ok;
}
int web_gpu_writeback_range(uint64_t address,uint64_t size) {
    pthread_mutex_lock(&resource_lock);Resource *found=NULL;
    for(unsigned i=0;i<4096&&!found;i++)for(Resource *r=resources[i];r;r=r->next)
        if(r->kind==POOL&&address>=r->address&&address-r->address<=r->size&&size<=r->size-(address-r->address)){found=r;break;}
    int ok=found&&found->gpu;
    /* GPU-only pools retain their physical bytes. CPU-backed pools must land
     * actual results before a later draw stages guest memory over this range. */
    if(ok&&found->memory&&!(found->flags&1))ok=transfer(found,address-found->address,size,1);
    pthread_mutex_unlock(&resource_lock);return ok;
}
int web_gpu_registered_texture(uint64_t key,unsigned index,WebGpuTextureBinding *output) {
    pthread_mutex_lock(&resource_lock);Resource *pool=find(key,TEXTURE_POOL);
    TextureSlot *slot=pool&&pool->textures?pool->textures[index&1023]:NULL;while(slot&&slot->index!=index)slot=slot->next;
    int found=slot!=NULL;if(found)*output=slot->value;
    pthread_mutex_unlock(&resource_lock);return found;
}

int web_gpu_descriptor_hold(uint64_t key,int sampler) {
    pthread_mutex_lock(&resource_lock);Resource *r=find(key,sampler?SAMPLER_POOL:TEXTURE_POOL);
    int valid=r&&r->references!=UINT32_MAX;if(valid)r->references++;
    pthread_mutex_unlock(&resource_lock);return valid;
}
void web_gpu_descriptor_release(uint64_t key,int sampler) {
    pthread_mutex_lock(&resource_lock);Resource *r=find(key,sampler?SAMPLER_POOL:TEXTURE_POOL);
    if(!r||!r->references)fail("invalid descriptor pool reference",key);
    r->references--;pthread_mutex_unlock(&resource_lock);
}
int web_gpu_resources_import(Ctx *c,const char *name) {
    unsigned descriptor_kind=!strncmp(name,"nvnTexturePool",14)?TEXTURE_POOL:!strncmp(name,"nvnSamplerPool",14)?SAMPLER_POOL:0;
    if(descriptor_kind) {
        const char *operation=name+14;
        int register_sampler=descriptor_kind==SAMPLER_POOL&&(!strcmp(operation,"RegisterSampler")||!strcmp(operation,"RegisterSamplerBuilder"));
        int register_texture=descriptor_kind==TEXTURE_POOL&&(!strcmp(operation,"RegisterTexture")||!strcmp(operation,"RegisterImage"));
        if(strcmp(operation,"Initialize")&&strcmp(operation,"Finalize")&&strcmp(operation,"GetMemoryPool")&&
           strcmp(operation,"GetMemoryOffset")&&strcmp(operation,"GetSize")&&strcmp(operation,"SetDebugLabel")&&!register_sampler&&!register_texture)return 0;
        WebGpuTextureRef *texture=NULL;WebGpuTextureBinding binding={0};
        if(register_texture&&c->x[2]) {
            texture=web_gpu_texture_hold(c->x[2]);if(!texture)guest_trap("invalid descriptor texture",c->x[2]);
            web_gpu_texture_snapshot(texture,&binding.builder,&binding.texture);binding.image=!strcmp(operation,"RegisterImage");
            if(c->x[3]&&gm_read(web_memory_current(),c->x[3],&binding.view,sizeof binding.view)!=GM_OK) {
                web_gpu_texture_release(texture);guest_trap("unmapped texture descriptor view",c->x[3]);
            }
            if((binding.view.set&0x90)!=0x90&&binding.builder.dsmode){binding.view.set|=0x90;binding.view.dsmode=(uint8_t)binding.builder.dsmode;}
        }
        pthread_mutex_lock(&resource_lock);Resource *descriptor=find(c->x[0],descriptor_kind);
        if(register_texture) {
            if(!descriptor||c->x[1]>=descriptor->size) {
                if(texture)web_gpu_texture_release(texture);fail("invalid texture descriptor registration",c->x[1]);
            }
            if(!descriptor->textures)descriptor->textures=calloc(1024,sizeof *descriptor->textures);
            if(!descriptor->textures){if(texture)web_gpu_texture_release(texture);fail("texture descriptor table allocation failed",descriptor->key);}
            TextureSlot **link=&descriptor->textures[c->x[1]&1023];while(*link&&(*link)->index!=c->x[1])link=&(*link)->next;
            TextureSlot *slot=*link;
            if(texture) {
                WebGpuReply reply=web_gpu_request(18,binding.texture,0,0,&binding.view,sizeof binding.view);
                if(reply.status){web_gpu_texture_release(texture);fail("WebGPU texture view registration failed",c->x[2]);}
                binding.view_handle=reply.value;web_gpu_texture_release(texture);
                if(!slot){slot=calloc(1,sizeof *slot);if(!slot){web_gpu_request(19,binding.view_handle,0,0,NULL,0);fail("texture descriptor allocation failed",descriptor->key);}slot->index=(unsigned)c->x[1];*link=slot;}
                if(slot->value.view_handle&&web_gpu_request(19,slot->value.view_handle,0,0,NULL,0).status)fail("old texture view release failed",slot->value.view_handle);
                slot->value=binding;
            } else if(slot) {
                if(slot->value.view_handle&&web_gpu_request(19,slot->value.view_handle,0,0,NULL,0).status)fail("texture view release failed",slot->value.view_handle);
                *link=slot->next;free(slot);
            }
            pthread_mutex_unlock(&resource_lock);return 1;
        }
        if(!strcmp(operation,"Initialize")) {
            Resource *pool=find(c->x[1],POOL);uint64_t offset=c->x[2],count=c->x[3];
            uint64_t maximum=descriptor_kind==TEXTURE_POOL?1048576:4096;
            if(descriptor||!pool||!count||count>maximum||offset%32||offset>pool->size||count>(pool->size-offset)/32) {
                snprintf(last_initialization_error,sizeof last_initialization_error,"descriptor count=%llx offset=%llx",(unsigned long long)count,(unsigned long long)offset);
                c->x[0]=0;pthread_mutex_unlock(&resource_lock);return 1;
            }
            descriptor=create(c->x[0],descriptor_kind);
            if(descriptor){descriptor->pool=pool->key;descriptor->offset=offset;descriptor->size=count;pool->references++;}
            c->x[0]=descriptor!=NULL;
        } else {
            if(!descriptor)fail("invalid NVN descriptor pool",c->x[0]);
            if(register_sampler) {
                if(c->x[1]>=descriptor->size||!c->x[2])fail("invalid sampler descriptor registration",c->x[1]);
                SamplerSlot *slot=descriptor->samplers;while(slot&&slot->index!=c->x[1])slot=slot->next;
                if(!slot){slot=calloc(1,sizeof *slot);if(!slot)fail("sampler descriptor allocation failed",descriptor->key);
                    slot->index=(unsigned)c->x[1];slot->next=descriptor->samplers;descriptor->samplers=slot;}
                web_read(c->x[2],&slot->value,sizeof slot->value);
            } else if(!strcmp(operation,"Finalize")) {
                if(descriptor->references)fail("finalising a descriptor pool referenced by recorded commands",descriptor->key);
                Resource *pool=find(descriptor->pool,POOL);
                if(!pool)fail("descriptor memory pool no longer exists",descriptor->pool);
                SamplerSlot *slot=descriptor->samplers;while(slot){SamplerSlot *next=slot->next;free(slot);slot=next;}
                if(descriptor->textures) {
                    for(unsigned i=0;i<1024;i++)for(TextureSlot *entry=descriptor->textures[i];entry;) {
                        TextureSlot *next=entry->next;
                        if(web_gpu_request(19,entry->value.view_handle,0,0,NULL,0).status)fail("descriptor view release failed",entry->value.view_handle);
                        free(entry);entry=next;
                    }
                    free(descriptor->textures);
                }
                pool->references--;drop(descriptor);
            } else if(!strcmp(operation,"GetMemoryPool"))c->x[0]=descriptor->pool;
            else if(!strcmp(operation,"GetMemoryOffset"))c->x[0]=descriptor->offset;
            else if(!strcmp(operation,"GetSize"))c->x[0]=descriptor->size;
        }
        pthread_mutex_unlock(&resource_lock);return 1;
    }
    int pool_builder=!strncmp(name,"nvnMemoryPoolBuilder",20);
    int buffer_builder=!strncmp(name,"nvnBufferBuilder",16);
    if(pool_builder||buffer_builder) {
        const char *operation=name+(pool_builder?20:16);uint64_t builder=c->x[0];
        if(!strcmp(operation,"SetDefaults")){
            uint64_t dev=rd64(builder+B_DEVICE)==web_gpu_device_address()?web_gpu_device_address():0;
            for(unsigned i=0;i<64;i+=8)wr64(builder+i,0);
            wr64(builder+B_DEVICE,dev);return 1;
        }
        if(!strcmp(operation,"SetDevice")) {
            if(c->x[1]!=web_gpu_device_address()||!c->x[1])guest_trap("invalid builder device",c->x[1]);
            wr64(builder+B_DEVICE,c->x[1]);return 1;
        }
        if(!strcmp(operation,"GetDevice")){c->x[0]=rd64(builder+B_DEVICE);return 1;}
        if(!strcmp(operation,"SetStorage")) {
            wr64(builder+B_STORAGE,c->x[1]);
            if(pool_builder)wr64(builder+B_SIZE,c->x[2]);
            else {wr64(builder+B_OFFSET,c->x[2]);wr64(builder+B_SIZE,c->x[3]);}
            return 1;
        }
        if(pool_builder&&!strcmp(operation,"SetFlags")){wr32(builder+B_FLAGS,(uint32_t)c->x[1]);return 1;}
        if(pool_builder&&!strcmp(operation,"GetFlags")){c->x[0]=rd32(builder+B_FLAGS);return 1;}
        if((pool_builder&&!strcmp(operation,"GetMemory"))||(buffer_builder&&!strcmp(operation,"GetMemoryPool"))){c->x[0]=rd64(builder+B_STORAGE);return 1;}
        if(!strcmp(operation,"GetSize")){c->x[0]=rd64(builder+B_SIZE);return 1;}
        if(buffer_builder&&!strcmp(operation,"GetMemoryOffset")){c->x[0]=rd64(builder+B_OFFSET);return 1;}
        return 0;
    }
    unsigned kind=!strncmp(name,"nvnMemoryPool",13)?POOL:!strncmp(name,"nvnBuffer",9)?BUFFER:0;
    if(!kind)return 0;
    const char *operation=name+(kind==POOL?13:9);
    // Only operations with complete semantics enter this locked dispatcher.
    const char *known[]={"Initialize","Finalize","Map","GetSize","GetBufferAddress","GetAddress","GetFlags","GetMemoryPool","GetMemoryOffset","FlushMappedRange","InvalidateMappedRange","GetDebugID","SetDebugLabel"};
    unsigned recognised=0;for(unsigned i=0;i<sizeof known/sizeof *known;i++)if(!strcmp(operation,known[i]))recognised=1;
    if(!recognised)return 0;
    pthread_mutex_lock(&resource_lock);Resource *r=find(c->x[0],kind);
    if(!strcmp(operation,"Initialize")) {
        uint64_t builder=c->x[1];
        if(r||rd64(builder+B_DEVICE)!=web_gpu_device_address()){
            snprintf(last_initialization_error,sizeof last_initialization_error,"existing=%d device=%llx",r!=NULL,(unsigned long long)rd64(builder+B_DEVICE));
            c->x[0]=0;pthread_mutex_unlock(&resource_lock);return 1;
        }
        uint64_t storage=rd64(builder+B_STORAGE),size=rd64(builder+B_SIZE),offset=rd64(builder+B_OFFSET);
        uint32_t flags=rd32(builder+B_FLAGS);
        if(kind==POOL) {
            if(flags&0x200)fail("virtual NVN memory pools require GPU alias mapping",builder);
            // Physical NVN pools can use 4 KiB allocations; the device's
            // 64 KiB virtual mapping page is not their allocation granularity.
            if(!size||size>0x7fffffff||size%4096||(storage&&!mapped(storage,size))||(!storage&&!(flags&1))){
                snprintf(last_initialization_error,sizeof last_initialization_error,"pool size=%llx memory=%llx flags=%x",(unsigned long long)size,(unsigned long long)storage,flags);
                c->x[0]=0;pthread_mutex_unlock(&resource_lock);return 1;
            }
            r=create(c->x[0],POOL);
            if(r){r->memory=storage;r->size=size;r->flags=flags;r->address=storage?storage:gpu_only_address;}
            if(r&&!storage) {
                gpu_only_address+=size;
                if(!ensure_gpu(r)){drop(r);r=NULL;}
            }
        } else {
            Resource *pool=find(storage,POOL);
            if(!pool||!size||offset>pool->size||size>pool->size-offset||!ensure_gpu(pool)){
                snprintf(last_initialization_error,sizeof last_initialization_error,"buffer pool=%llx offset=%llx size=%llx",(unsigned long long)storage,(unsigned long long)offset,(unsigned long long)size);
                c->x[0]=0;pthread_mutex_unlock(&resource_lock);return 1;
            }
            r=create(c->x[0],BUFFER);
            if(r){r->pool=storage;r->offset=offset;r->size=size;r->memory=pool->memory?pool->memory+offset:0;r->address=pool->address+offset;r->gpu=pool->gpu;pool->references++;}
        }
        c->x[0]=r!=NULL;pthread_mutex_unlock(&resource_lock);return 1;
    }
    if(!r){char detail[160];snprintf(detail,sizeof detail,"invalid %s; %s",name,last_initialization_error);fail(detail,c->x[0]);}
    Resource *pool=kind==POOL?r:find(r->pool,POOL);
    if(!pool)fail("buffer's memory pool no longer exists",r->pool);
    if(!strcmp(operation,"Finalize")) {
        if(kind==POOL) {
            if(r->references)fail("finalising a pool with live buffers",r->key);
            if(r->gpu&&web_gpu_request(4,r->gpu,0,0,NULL,0).status)fail("GPU resource release failed",r->key);
        } else pool->references--;
        drop(r);
    } else if(!strcmp(operation,"Map"))c->x[0]=(pool->flags&1)?0:r->memory;
    else if(!strcmp(operation,"GetSize"))c->x[0]=r->size;
    else if(!strcmp(operation,"GetAddress")||!strcmp(operation,"GetBufferAddress"))c->x[0]=r->address;
    else if(!strcmp(operation,"GetFlags"))c->x[0]=r->flags;
    else if(!strcmp(operation,"GetMemoryPool"))c->x[0]=r->pool;
    else if(!strcmp(operation,"GetMemoryOffset"))c->x[0]=r->offset;
    else if(!strcmp(operation,"GetDebugID"))c->x[0]=r->key;
    else if(!strcmp(operation,"SetDebugLabel")) { /* Diagnostic label only; guest storage remains opaque. */ }
    else {
        uint64_t offset=c->x[1],size=c->x[2];
        if(offset>r->size||size>r->size-offset)fail("NVN cache range outside resource",offset);
        if(!transfer(pool,r->offset+offset,size,!strcmp(operation,"InvalidateMappedRange")))fail("GPU resource transfer failed",r->key);
    }
    pthread_mutex_unlock(&resource_lock);return 1;
}
