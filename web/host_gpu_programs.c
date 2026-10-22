/* Program source ownership. Native shader bytes are copied before SetShaders
 * returns; topology/resource-specific compilation belongs to pipeline creation. */
#include "runtime.h"
#include "host_gpu_bridge.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
typedef struct Program {uint64_t key;uint32_t handle,sample_shading;char label[128];struct Program *next;} Program;
static Program *programs;
static pthread_mutex_t program_lock=PTHREAD_MUTEX_INITIALIZER;
int web_gpu_program_snapshot(uint64_t key,uint32_t *snapshot) {
    pthread_mutex_lock(&program_lock);
    Program *program=programs;while(program&&program->key!=key)program=program->next;
    WebGpuReply reply={1,0};
    if(program&&program->handle)reply=web_gpu_request(20,program->handle,0,0,NULL,0);
    pthread_mutex_unlock(&program_lock);
    if(reply.status||!reply.value)return 0;
    *snapshot=reply.value;return 1;
}
static void fail(const char *what,uint64_t address){pthread_mutex_unlock(&program_lock);guest_trap(what,address);}
static int copy_data(uint64_t address,void *output,size_t size) {
    if(gm_read(web_memory_current(),address,output,size)==GM_OK)return 1;
    uint32_t handle;uint64_t offset;
    if(!web_gpu_resolve(address,size,&handle,&offset))return 0;
    uint64_t start=offset&~3ULL;size_t length=(size_t)((offset-start+size+3)&~3ULL);
    void *staging=malloc(length);if(!staging)return 0;
    WebGpuReply reply=web_gpu_request(3,handle,start,length,staging,length);
    if(!reply.status)memcpy(output,(uint8_t *)staging+offset-start,size);
    free(staging);return !reply.status;
}
int web_gpu_programs_import(Ctx *c,const char *name) {
    if(strncmp(name,"nvnProgram",10))return 0;
    const char *operation=name+10;
    int set=!strcmp(operation,"SetShaders")||!strcmp(operation,"SetShadersExt");
    if(!set&&strcmp(operation,"Initialize")&&strcmp(operation,"Finalize")&&strcmp(operation,"SetDebugLabel")&&strcmp(operation,"SetSampleShading"))return 0;
    pthread_mutex_lock(&program_lock);Program **link=&programs;
    while(*link&&(*link)->key!=c->x[0])link=&(*link)->next;
    Program *program=*link;
    if(!strcmp(operation,"Initialize")) {
        if(program||!c->x[0]||c->x[1]!=web_gpu_device_address()){c->x[0]=0;pthread_mutex_unlock(&program_lock);return 1;}
        program=calloc(1,sizeof *program);
        if(program){program->key=c->x[0];program->next=programs;programs=program;}
        c->x[0]=program!=NULL;
    } else if(!program)fail("invalid NVN program",c->x[0]);
    else if(!strcmp(operation,"Finalize")) {
        if(program->handle&&web_gpu_request(13,program->handle,0,0,NULL,0).status)fail("GPU program release failed",program->key);
        *link=program->next;free(program);
    } else if(!strcmp(operation,"SetDebugLabel")) {
        unsigned i=0;if(c->x[1])for(;i<127;i++){program->label[i]=(char)rd8(c->x[1]+i);if(!program->label[i])break;}program->label[i]=0;
    } else if(!strcmp(operation,"SetSampleShading")) {
        program->sample_shading=(uint32_t)c->x[1];
    } else {
        uint64_t count=c->x[1],array=c->x[2],stride=!strcmp(operation,"SetShadersExt")?24:16;
        if(!count||count>6||!array||array>UINT64_MAX-count*stride)fail("invalid NVN shader array",array);
        uint64_t control[6],data[6];uint32_t controls[6],sizes[6];size_t total=4;
        for(unsigned i=0;i<count;i++) {
            data[i]=rd64(array+i*stride);control[i]=rd64(array+i*stride+8);
            if(!control[i]||control[i]>UINT64_MAX-0x75c||rd32(control[i])!=0x98761234)fail("invalid NVN shader control",control[i]);
            int compute=rd32(control[i]+0x714)==5;
            uint32_t code=rd32(control[i]+0x6f8),constant=rd32(control[i]+0x6fc),offset=rd32(control[i]+0x700);
            if(code<32||code>4*1024*1024||constant>1024*1024||offset>8*1024*1024||constant>8*1024*1024-offset)
                fail("NVN shader code/constants exceed limits",control[i]);
            controls[i]=compute?0x75c:0x720;sizes[i]=code+(compute?0:0x30);
            if(constant&&offset+constant>sizes[i])sizes[i]=offset+constant;
            if(!data[i]||data[i]>UINT64_MAX-sizes[i])fail("invalid NVN shader data address",data[i]);
            total+=8+controls[i]+sizes[i];
        }
        uint8_t *packet=malloc(total);if(!packet)fail("shader source allocation failed",program->key);
        uint32_t n=(uint32_t)count;memcpy(packet,&n,4);size_t position=4;
        for(unsigned i=0;i<count;i++) {
            memcpy(packet+position,&controls[i],4);memcpy(packet+position+4,&sizes[i],4);position+=8;
            if(gm_read(web_memory_current(),control[i],packet+position,controls[i])!=GM_OK){free(packet);fail("unmapped shader control",control[i]);}
            position+=controls[i];
            if(!copy_data(data[i],packet+position,sizes[i])){free(packet);fail("unmapped shader data",data[i]);}
            position+=sizes[i];
        }
        WebGpuReply reply=web_gpu_request(12,program->handle,0,total,packet,total);free(packet);
        if(reply.status)fail("GPU program source registration failed",program->key);
        program->handle=reply.value;c->x[0]=1;
    }
    pthread_mutex_unlock(&program_lock);return 1;
}
