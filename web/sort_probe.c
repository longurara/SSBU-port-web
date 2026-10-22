/* Synthetic input only; compare this adapter with the user's SDK oracle. */
#include "runtime.h"
#include <setjmp.h>
#include <stdlib.h>
int web_unsigned_sort_import(Ctx *,const char *);
static jmp_buf stopped;
void guest_trap(const char *what,uint64_t address){(void)what;(void)address;longjmp(stopped,1);}
int sort_probe(uint8_t *bytes,unsigned length,unsigned width) {
    if((width!=4&&width!=8)||length>60000||length%width)return 0;
    GuestMemory memory;gm_init(&memory);web_memory_bind(&memory);
    const uint64_t region=0x7300200000ULL,base=region+4092;
    int ok=0;
    if(!setjmp(stopped)&&gm_reserve(&memory,region,65536)==GM_OK&&gm_commit(&memory,region,65536)==GM_OK) {
        wr32(base-4,0xcafebabe);wr32(base+length,0x1234abcd);web_write(base,bytes,length);
        Ctx c={0};c.x[0]=base;c.x[1]=base+length;
        ok=web_unsigned_sort_import(&c,width==4?"_ZNSt3__16__sortIRNS_6__lessIjjEEPjEEvT0_S5_T_":"_ZNSt3__16__sortIRNS_6__lessIyyEEPyEEvT0_S5_T_");
        web_read(base,bytes,length);
        ok=ok&&rd32(base-4)==0xcafebabe&&rd32(base+length)==0x1234abcd;
    }
    web_memory_bind(NULL);gm_destroy(&memory);return ok;
}
