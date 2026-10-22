#include "host_fiber_tls.h"
static _Thread_local void *fiber_thread;
void *web_fiber_thread_state(void){return fiber_thread;}
void web_fiber_set_thread_state(void *state){fiber_thread=state;}
