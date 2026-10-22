#ifndef WEB_RUNTIME_MEMORY_H
#define WEB_RUNTIME_MEMORY_H
/* Included by runtime.h after Ctx and V128 are declared. */
#include "guest_memory.h"
void web_memory_bind(GuestMemory *memory);
GuestMemory *web_memory_current(void);
_Noreturn void web_memory_fault(const char *operation, GuestAddr address, size_t size, GmStatus status);
static inline void web_read(GuestAddr a, void *out, size_t n) {
    GuestMemory *m = web_memory_current();
    GmStatus s = m ? gm_read(m, a, out, n) : GM_UNMAPPED;
    if (s != GM_OK) web_memory_fault("read", a, n, s);
}
static inline void web_write(GuestAddr a, const void *data, size_t n) {
    GuestMemory *m = web_memory_current();
    GmStatus s = m ? gm_write(m, a, data, n) : GM_UNMAPPED;
    if (s != GM_OK) web_memory_fault("write", a, n, s);
}
#define DEF_WEB_MEM(bits) \
    static inline uint##bits##_t rd##bits(uint64_t a) { uint##bits##_t v; web_read(a, &v, sizeof v); return v; } \
    static inline void wr##bits(uint64_t a, uint##bits##_t v) { web_write(a, &v, sizeof v); }
DEF_WEB_MEM(8) DEF_WEB_MEM(16) DEF_WEB_MEM(32) DEF_WEB_MEM(64)
#undef DEF_WEB_MEM
#define DEF_WEB_STX(bits) \
    static inline uint32_t stx##bits(Ctx *c, uint64_t a, uint##bits##_t value) { \
        if (c->ex_addr != a) { stx_diag(c, a, 0, 0, value, -bits); return 1; } \
        c->ex_addr = UINT64_MAX; \
        uint64_t expected = (uint##bits##_t)c->ex_val; int changed = 0; \
        GuestMemory *m = web_memory_current(); \
        GmStatus status = m ? gm_cas(m, a, bits, &expected, value, &changed) : GM_UNMAPPED; \
        if (status != GM_OK) web_memory_fault("stx", a, bits / 8, status); \
        if (changed) { c->ex_fail = 0; return 0; } \
        stx_diag(c, a, c->ex_val, expected, value, bits); return 1; }
DEF_WEB_STX(8) DEF_WEB_STX(16) DEF_WEB_STX(32) DEF_WEB_STX(64)
#undef DEF_WEB_STX
static inline V128 rdv(uint64_t a, int n) {
    if (n < 0 || n > 16) web_memory_fault("read-vector-width", a, 0, GM_RANGE);
    V128 value = {0}; web_read(a, &value, (size_t)n); return value;
}
static inline void wrv(uint64_t a, const V128 *value, int n) {
    if (n < 0 || n > 16) web_memory_fault("write-vector-width", a, 0, GM_RANGE);
    web_write(a, value, (size_t)n);
}
#endif
