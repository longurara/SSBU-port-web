#ifndef WEB_RUNTIME_DIAGNOSTICS_H
#define WEB_RUNTIME_DIAGNOSTICS_H
#include <stddef.h>
#include <stdint.h>
/* Faults must not depend on stdio's indirect FILE callbacks: otherwise
 * Asyncify conservatively propagates suspension into every guest memory read.
 * These bounded formatters allocate nothing and invoke no callbacks. */
typedef struct {char *data;size_t capacity,used;} WebDiagnostic;
static inline void web_diag_char(WebDiagnostic *out,char value){
    if(out->capacity&&out->used<out->capacity-1)out->data[out->used++]=value;
}
static inline void web_diag_text(WebDiagnostic *out,const char *text){
    if(!text)text="(null)";
    while(out->capacity&&out->used<out->capacity-1&&*text)web_diag_char(out,*text++);
}
static inline void web_diag_unsigned(WebDiagnostic *out,uint64_t value,int hexadecimal){
    unsigned base=hexadecimal?16:10;
    char digits[20];unsigned count=0;
    do{unsigned digit=(unsigned)(value%base);digits[count++]=(char)(digit<10?'0'+digit:'a'+digit-10);value/=base;}while(value);
    while(count)web_diag_char(out,digits[--count]);
}
static inline void web_diag_finish(WebDiagnostic *out){if(out->capacity)out->data[out->used]=0;}
static inline void web_copy_diagnostic(char *data,size_t capacity,const char *text){
    WebDiagnostic out={data,capacity,0};web_diag_text(&out,text);web_diag_finish(&out);
}
static inline void web_format_memory_fault(char *data,size_t capacity,const char *operation,const char *status,size_t size){
    WebDiagnostic out={data,capacity,0};
    web_diag_text(&out,"memory ");web_diag_text(&out,operation);web_diag_text(&out," (");web_diag_text(&out,status);
    web_diag_text(&out,"), ");web_diag_unsigned(&out,size,0);web_diag_text(&out," bytes");web_diag_finish(&out);
}
static inline void web_format_unframed_trap(char *data,size_t capacity,const char *message,uint64_t address){
    WebDiagnostic out={data,capacity,0};
    web_diag_text(&out,"guest trap outside runtime frame: ");web_diag_text(&out,message);web_diag_text(&out," (");
    web_diag_unsigned(&out,address,1);web_diag_text(&out,")");web_diag_finish(&out);
}
#endif
