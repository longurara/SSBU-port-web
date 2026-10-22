#include "runtime_frame.h"
static _Thread_local void *active_frame;
void *web_runtime_frame_get(void){return active_frame;}
void web_runtime_frame_set(void *frame){active_frame=frame;}
