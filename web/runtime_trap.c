#include "runtime_frame.h"
#include "runtime_diagnostics.h"
#include <emscripten.h>
#include <stdlib.h>
EM_JS_DEPS(runtime_trap_deps,"$UTF8ToString");
EM_JS(void,unframed_trap_log,(const char *message),{console.error(UTF8ToString(message));});
void guest_trap(const char *what,uint64_t address){
    WebRuntimeFrame *frame=web_runtime_frame_get();
    if(!frame){
        char message[224];web_format_unframed_trap(message,sizeof message,what,address);
        unframed_trap_log(message);abort();
    }
    web_copy_diagnostic(frame->failure,sizeof frame->failure,what);
    frame->failure_address=address;
    longjmp(frame->boundary,1);
}
