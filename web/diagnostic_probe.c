/* Compare fault formatting to libc and test the production trap/frame path. */
#include "runtime.h"
#include "runtime_frame.h"
#include "runtime_diagnostics.h"
#include <emscripten/fiber.h>
#include <stdio.h>
#include <string.h>
static unsigned checks,failures;
static WebRuntimeFrame frame,parent;
static GuestMemory memory;
static const char long_message[]=
 "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
 "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
 "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
 "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
 "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static void check(int value){checks++;if(!value)failures++;}
unsigned diagnostic_count(void){return checks;}
int diagnostic_check(void){
    char actual[256],expected[256];checks=failures=0;
    const char *ops[]={"read","write","stx","read-vector-width"};
    const char *messages[]={"","plain message","UTF-8: đọc bộ nhớ",long_message};
    const uint64_t numbers[]={0,1,8,4096,UINT32_MAX,UINT64_C(0x100000000),UINT64_MAX};
    for(size_t cap=0;cap<=240;cap++){
        for(unsigned op=0;op<4;op++)for(unsigned status=0;status<9;status++)for(unsigned n=0;n<7;n++){
            memset(actual,0xcc,sizeof actual);memset(expected,0xcc,sizeof expected);
            size_t value=(size_t)numbers[n];
            snprintf(expected+8,cap,"memory %s (%s), %zu bytes",ops[op],gm_status_name((GmStatus)status),value);
            web_format_memory_fault(actual+8,cap,ops[op],gm_status_name((GmStatus)status),value);
            check(!memcmp(actual,expected,sizeof actual));
        }
        for(unsigned m=0;m<4;m++){
            memset(actual,0xcc,sizeof actual);memset(expected,0xcc,sizeof expected);
            snprintf(expected+8,cap,"%s",messages[m]);web_copy_diagnostic(actual+8,cap,messages[m]);check(!memcmp(actual,expected,sizeof actual));
            for(unsigned n=0;n<7;n++){
                memset(actual,0xcc,sizeof actual);memset(expected,0xcc,sizeof expected);
                snprintf(expected+8,cap,"guest trap outside runtime frame: %s (%llx)",messages[m],(unsigned long long)numbers[n]);
                web_format_unframed_trap(actual+8,cap,messages[m],numbers[n]);check(!memcmp(actual,expected,sizeof actual));
            }
        }
    }
    web_copy_diagnostic(NULL,0,"unused");web_format_memory_fault(NULL,0,"read","unmapped",8);web_format_unframed_trap(NULL,0,"unused",0);
    return (int)failures;
}
uint64_t diagnostic_read(uint64_t address){return rd64(address);}
void diagnostic_write(uint64_t address,uint64_t value){wr64(address,value);}
/* Keep a genuine suspension path in the module, so the analyzer must still
 * instrument it while proving that memory helpers cannot suspend. */
void diagnostic_switch(emscripten_fiber_t *old,emscripten_fiber_t *next){emscripten_fiber_swap(old,next);}
int diagnostic_memory(void){
    gm_init(&memory);web_memory_bind(&memory);memset(&frame,0,sizeof frame);memset(&parent,0,sizeof parent);
    parent.import_symbol=987;strcpy(parent.failure,"parent unchanged");frame.previous=&parent;frame.import_symbol=7;web_runtime_frame_set(&frame);
    const uint64_t address=UINT64_C(0x7100001000),unmapped=UINT64_C(0x7100003000),value=UINT64_C(0x123456789abcdef0);
    if(gm_reserve(&memory,address,4096)!=GM_OK||gm_commit(&memory,address,4096)!=GM_OK)return 1;
    diagnostic_write(address,value);if(diagnostic_read(address)!=value)return 2;
    if(!setjmp(frame.boundary)){diagnostic_read(unmapped);return 3;}
    if(frame.failure_address!=unmapped||strcmp(frame.failure,"memory read (unmapped), 8 bytes"))return 4;
    if(gm_protect(&memory,address,4096,0)!=GM_OK)return 5;
    if(!setjmp(frame.boundary)){diagnostic_write(address,0);return 6;}
    if(frame.failure_address!=address||strcmp(frame.failure,"memory write (readonly), 8 bytes")||diagnostic_read(address)!=value)return 7;
    if(!setjmp(frame.boundary)){guest_trap(long_message,UINT64_MAX);return 8;}
    char expected[160];snprintf(expected,sizeof expected,"%s",long_message);
    if(frame.failure_address!=UINT64_MAX||strcmp(frame.failure,expected)||frame.previous!=&parent||frame.import_symbol!=7||parent.import_symbol!=987||strcmp(parent.failure,"parent unchanged"))return 9;
    web_runtime_frame_set(NULL);web_memory_bind(NULL);gm_destroy(&memory);return 0;
}
