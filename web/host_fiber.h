#ifndef WEB_HOST_FIBER_H
#define WEB_HOST_FIBER_H
#include "runtime.h"
int web_fiber_import(Ctx *context,const char *name);
void web_fiber_thread_cleanup(void);
#endif
