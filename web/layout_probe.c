#include "host_gpu_texture.h"
#include <setjmp.h>
#include <stdio.h>
static GuestMemory memory;
static int ready;
static jmp_buf stopped;
static char error[192];
void guest_trap(const char *what,uint64_t address){snprintf(error,sizeof error,"%s (%llx)",what,(unsigned long long)address);longjmp(stopped,1);}
void guest_unimpl(uint64_t address,uint32_t instruction,const char *what){(void)instruction;guest_trap(what,address);}
const char *layout_probe_error(void){return error;}
int layout_probe(const uint32_t *fields,uint64_t *output) {
    error[0]=0;if(setjmp(stopped))return 0;
    if(!ready) {
        if(gm_init_limit(&memory,4096)!=GM_OK||gm_reserve(&memory,0x7300100000ULL,65536)!=GM_OK||
           gm_commit(&memory,0x7300100000ULL,65536)!=GM_OK)return 0;
        ready=1;
    }
    web_memory_bind(&memory);
    WebGpuTextureBuilder b={0};
    b.flags=fields[0];b.target=fields[1];b.width=fields[2];b.height=fields[3];b.depth=fields[4];b.levels=fields[5];
    b.format=fields[6];b.samples=fields[7];b.stride=fields[8];b.glname=fields[9];b.has_layout=fields[10];
    for(unsigned i=0;i<4;i++)b.layout[i]=(uint8_t)(fields[11]>>(i*8));
    Ctx c={0};c.x[31]=0x730010fff0ULL;
    const unsigned functions[]={0x4278b0,0x428760,0x427a10,0x427b30,0x4290b0};
    for(unsigned i=0;i<5;i++) {
        uint32_t sparse[4]={0};uint64_t value=web_nvn_layout_query(&c,&b,functions[i],sparse);
        if(i<4)output[i]=value;else for(unsigned j=0;j<4;j++)output[4+j]=sparse[j];
    }
    web_memory_bind(NULL);return 1;
}
