/* First real bootstrap slice. Only argv and FS allocator registration exist at
 * this stage; all other host calls stop rather than silently returning zero. */
#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>

GuestMemory *elf_memory(void);
void f_71002c55e0(Ctx *c);
static unsigned calls[3];
static uint64_t fs_allocate, fs_free;
static char result[512];
static Ctx last_context;
void startup_copy_context(Ctx *out) { *out = last_context; }
#ifndef WEB_FULL_RUNTIME
void guest_trap(const char *what, uint64_t address) {
    fprintf(stderr, "bootstrap stopped: %s at %llx\n", what, (unsigned long long)address);
    abort();
}
void guest_unimpl(uint64_t pc, uint32_t word, const char *what) { (void)word; guest_trap(what, pc); }
void guest_svc(Ctx *c, uint32_t number) { (void)c; guest_trap("unported svc", number); }
void call_indirect(Ctx *c, uint64_t address) { (void)c; guest_trap("unported indirect function", address); }
#endif
int startup_host_import(Ctx *c, int symbol) {
    switch (symbol) {
    case 10: /* nn::os::GetHostArgc(): browser launch supplies no CLI args */
        calls[0]++; c->x[0] = 0; return 1;
    case 11: /* nn::os::GetHostArgv(): argc == 0 */
        calls[1]++; c->x[0] = 0; return 1;
    case 1249: /* nn::fs::SetAllocator(allocate, free): preserve guest callbacks */
        calls[2]++; fs_allocate = c->x[0]; fs_free = c->x[1]; return 1;
    default: return 0;
    }
}
#ifndef WEB_FULL_RUNTIME
void host_import(Ctx *c, int symbol) {
    if (!startup_host_import(c, symbol)) guest_trap("unported host import", (uint64_t)symbol);
}
void stx_diag(Ctx *c, uint64_t a, uint64_t wanted, uint64_t seen, uint64_t value, int bits) {
    (void)c; (void)a; (void)wanted; (void)seen; (void)value; (void)bits;
}
#endif
const char *startup_info(void) { return result; }
int startup_run(void) {
    GuestMemory *memory = elf_memory();
    if (!memory) { snprintf(result, sizeof result, "{\"ok\":false,\"error\":\"ELF not loaded\"}"); return 0; }
    const uint64_t stack = 0x7300100000ULL, size = 1048576;
    if (gm_reserve(memory, stack, size) != GM_OK || gm_commit(memory, stack, (size_t)size) != GM_OK) {
        snprintf(result, sizeof result, "{\"ok\":false,\"error\":\"Cannot map bootstrap stack\"}"); return 0;
    }
    memset(calls, 0, sizeof calls); fs_allocate = fs_free = 0;
    Ctx c = {0}; c.x[31] = stack + size - 16; c.x[30] = 0x7300000000ULL;
    web_memory_bind(memory);
    f_71002c55e0(&c);
    last_context = c;
    int ok = c.x[31] == stack + size - 16 && c.x[30] == 0x7300000000ULL &&
             calls[0] == 1 && calls[1] == 1 && calls[2] == 1 &&
             fs_allocate == 0x710353bba0ULL && fs_free == 0x710353bc10ULL &&
             rd64(0x71052b5f18ULL) == 0x710437ad07ULL;
    web_memory_bind(NULL);
    snprintf(result, sizeof result, "{\"ok\":%s,\"phase\":\"nninitStartup\",\"importsHandled\":3,\"guestStackRestored\":%s,\"fsAllocator\":\"%llx\",\"fsFree\":\"%llx\",\"constructorsRun\":false,\"nnMainRun\":false}",
             ok ? "true" : "false", c.x[31] == stack + size - 16 ? "true" : "false", (unsigned long long)fs_allocate, (unsigned long long)fs_free);
    return ok;
}
