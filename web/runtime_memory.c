#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef WEB_FULL_RUNTIME
#include "runtime_diagnostics.h"
#endif
static _Thread_local GuestMemory *bound_memory;
void web_memory_bind(GuestMemory *m) { bound_memory = m; }
GuestMemory *web_memory_current(void) { return bound_memory; }
_Noreturn void web_memory_fault(const char *operation, GuestAddr address, size_t size, GmStatus status) {
#ifdef WEB_FULL_RUNTIME
    char message[128];
    web_format_memory_fault(message,sizeof message,operation,gm_status_name(status),size);
    guest_trap(message, address);
#else
    fprintf(stderr, "web memory fault: %s addr=0x%llx size=%zu status=%s\n",
            operation, (unsigned long long)address, size, gm_status_name(status));
#endif
    abort();
}
