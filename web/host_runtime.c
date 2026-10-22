/* Portable host layer for the actual constructor graph. Unsupported services
 * stop at a recoverable diagnostic boundary; they never return fake success. */
#define _POSIX_C_SOURCE 200809L
#include "runtime.h"
#include "hostrt.h"
#include "runtime_frame.h"
#include "host_mutex.h"
#include "host_semaphore.h"
#ifdef WEB_FIBERS
#include "host_fiber.h"
#endif
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <setjmp.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

GuestMemory *elf_memory(void);
int startup_run(void);
void startup_copy_context(Ctx *);
int startup_host_import(Ctx *, int);
int elf_relocate(void);
int web_string_import(Ctx *, const char *);
int web_unsigned_sort_import(Ctx *, const char *);
int web_os_import(Ctx *, const char *);
int web_os_indirect(Ctx *, uint64_t);
int web_fs_import(Ctx *, const char *);
int web_gpu_import(Ctx *, const char *);
int web_gpu_indirect(Ctx *, uint64_t);
void web_os_reset(void);
int runtime_main(void);
static pthread_t driver_thread;
static int driver_started;
extern const uint64_t constructor_addresses[];
extern const unsigned constructor_count;
extern const ImportDef web_sdk_leaf_imports[];
extern const unsigned web_sdk_leaf_count;
typedef WebRuntimeFrame RuntimeFrame;
static RuntimeFrame *active_frame(void){return web_runtime_frame_get();}
void web_runtime_frame_discard(void *pointer){
    RuntimeFrame *frame=pointer;
    while(frame){RuntimeFrame *previous=frame->previous;free(frame);frame=previous;}
}
static Ctx context;
static unsigned constructor_index, completed;
static atomic_uint import_calls;
static int constructors_ready;
static char result[1024], thread_failure[1024];
static atomic_uint thread_failed;
static uint64_t *data_bindings;
static uint64_t next_region = 0x8000000000ULL;
static const uint64_t errno_address = 0x7800000000ULL;
static const uint64_t main_thread = 0x7800000100ULL;
static const uint64_t main_tls = 0x7800010000ULL;
static int main_priority = 16;
static int main_core;
static uint64_t main_core_mask = 1;
#define CXX_ID_CTYPE 0x7600008000ULL
#define CXX_ID_NUMPUNCT 0x7600008010ULL
#define CXX_FUTURE_CAT 0x7600008020ULL
#define CXX_VT_CTYPE 0x7600008100ULL
#define CXX_CTYPE_OBJ 0x7600008200ULL
#define CXX_CLASSIC_TABLE 0x7600008300ULL
#define CXX_VT_NUMPUNCT 0x7600008600ULL
#define CXX_NUMPUNCT_OBJ 0x7600008700ULL
#define CXX_VT_ASSOC 0x7600008800ULL
#define CXX_VT_OSTREAM 0x7600008900ULL
#define CXX_VT_ISTREAM 0x7600008a00ULL
#define CXX_FN_BASE 0x7500003000ULL

static const uint16_t cxx_classic_table[256] = {
    0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x2003, 0x2002, 0x2002, 0x2002, 0x2002, 0x002, 0x002,
    0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002, 0x002,
    0x6001, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004,
    0x5800, 0x5800, 0x5800, 0x5800, 0x5800, 0x5800, 0x5800, 0x5800, 0x5800, 0x5800, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004,
    0x4004, 0x5500, 0x5500, 0x5500, 0x5500, 0x5500, 0x5500, 0x4500, 0x4500, 0x4500, 0x4500, 0x4500, 0x4500, 0x4500, 0x4500, 0x4500,
    0x4500, 0x4500, 0x4500, 0x4500, 0x4500, 0x4500, 0x4500, 0x4500, 0x4500, 0x4500, 0x4500, 0x4004, 0x4004, 0x4004, 0x4004, 0x4004,
    0x4004, 0x5600, 0x5600, 0x5600, 0x5600, 0x5600, 0x5600, 0x4600, 0x4600, 0x4600, 0x4600, 0x4600, 0x4600, 0x4600, 0x4600, 0x4600,
    0x4600, 0x4600, 0x4600, 0x4600, 0x4600, 0x4600, 0x4600, 0x4600, 0x4600, 0x4600, 0x4600, 0x4004, 0x4004, 0x4004, 0x4004, 0x002,
};

uint64_t runtime_import_address(unsigned symbol) {
    return data_bindings && symbol < (unsigned)dyn_sym_count ? data_bindings[symbol] : 0;
}
static void bind_sym_addr(const char *name, uint64_t addr) {
    for (int s = 0; s < dyn_sym_count; s++)
        if (!strcmp(dyn_syms[s].name, name)) data_bindings[s] = addr;
}
static void bind_sdk_data(void) {
    free(data_bindings);
    data_bindings = calloc((size_t)dyn_sym_count, sizeof *data_bindings);
    if (!data_bindings) guest_trap("data binding allocation failed", 0);
    GuestMemory *memory = elf_memory();
    next_region = 0x8000000000ULL;
    if (gm_reserve(memory, errno_address, 16 * GM_PAGE_SIZE) != GM_OK || gm_commit(memory, errno_address, 16 * GM_PAGE_SIZE) != GM_OK)
        guest_trap("cannot map errno state", errno_address);
    if (gm_reserve(memory, main_tls, 256 * 65536) != GM_OK || gm_commit(memory, main_tls, 65536) != GM_OK)
        guest_trap("cannot map main TLS", main_tls);
    if (gm_write(memory, main_tls + 0x1f8, &main_thread, 8) != GM_OK) guest_trap("cannot initialise main ThreadType", main_tls);
    const uint64_t base = 0x7600000000ULL;
    size_t mapped = 16 * GM_PAGE_SIZE;
    if (gm_reserve(memory, base, mapped) != GM_OK || gm_commit(memory, base, mapped) != GM_OK)
        guest_trap("cannot map SDK data constants", base);
    size_t offset = 0;
    for (int i = 0; i < sdk_data_count; i++) {
        if (gm_write(memory, base + offset, sdk_data[i].bytes, sdk_data[i].size) != GM_OK) guest_trap("SDK data copy failed", base + offset);
        bind_sym_addr(sdk_data[i].name, base + offset);
        offset += (sdk_data[i].size + 15u) & ~15u;
    }
    const char *streams[] = {"stdin", "stdout", "stderr"};
    for (unsigned i = 0; i < 3; i++) {
        uint64_t handle = 0x7700000000ULL + (uint64_t)(i + 1) * 16;
        if (gm_write(memory, base + offset, &handle, 8) != GM_OK) guest_trap("stdio binding failed", base + offset);
        bind_sym_addr(streams[i], base + offset);
        offset += 8;
    }
    gm_write(memory, CXX_CLASSIC_TABLE, cxx_classic_table, sizeof cxx_classic_table);
    for (unsigned i = 0; i < 11; i++) {
        uint64_t fn = CXX_FN_BASE + (uint64_t)i * 16;
        gm_write(memory, CXX_VT_CTYPE + 16 + (uint64_t)i * 8, &fn, 8);
    }
    uint64_t ctype_obj[4] = {CXX_VT_CTYPE + 16, 0x7fffffffULL, CXX_CLASSIC_TABLE, 0};
    gm_write(memory, CXX_CTYPE_OBJ, ctype_obj, sizeof ctype_obj);
    for (unsigned i = 0; i < 8; i++) {
        uint64_t fn = CXX_FN_BASE + 0x100ULL + (uint64_t)i * 16;
        gm_write(memory, CXX_VT_NUMPUNCT + 16 + (uint64_t)i * 8, &fn, 8);
    }
    uint64_t numpunct_obj[6] = {CXX_VT_NUMPUNCT + 16, 0x7fffffffULL, 0x2c2eULL, 0, 0, 0};
    gm_write(memory, CXX_NUMPUNCT_OBJ, numpunct_obj, sizeof numpunct_obj);
    uint64_t vt_assoc[6] = {0, 0, CXX_FN_BASE + 0x200ULL, CXX_FN_BASE + 0x210ULL, CXX_FN_BASE + 0x220ULL, CXX_FN_BASE + 0x230ULL};
    gm_write(memory, CXX_VT_ASSOC, vt_assoc, sizeof vt_assoc);
    uint64_t vt_ostream[10] = {8, 0, 0, CXX_FN_BASE + 0x300ULL, CXX_FN_BASE + 0x310ULL, (uint64_t)-8LL, (uint64_t)-8LL, 0, CXX_FN_BASE + 0x320ULL, CXX_FN_BASE + 0x330ULL};
    gm_write(memory, CXX_VT_OSTREAM, vt_ostream, sizeof vt_ostream);
    uint64_t vt_istream[10] = {16, 0, 0, CXX_FN_BASE + 0x340ULL, CXX_FN_BASE + 0x350ULL, (uint64_t)-16LL, (uint64_t)-16LL, 0, CXX_FN_BASE + 0x320ULL, CXX_FN_BASE + 0x330ULL};
    gm_write(memory, CXX_VT_ISTREAM, vt_istream, sizeof vt_istream);
    bind_sym_addr("_ZNSt3__15ctypeIcE2idE", CXX_ID_CTYPE);
    bind_sym_addr("_ZNSt3__18numpunctIcE2idE", CXX_ID_NUMPUNCT);
    bind_sym_addr("_ZTVNSt3__117__assoc_sub_stateE", CXX_VT_ASSOC);
    bind_sym_addr("_ZTVNSt3__113basic_ostreamIcNS_11char_traitsIcEEEE", CXX_VT_OSTREAM);
    bind_sym_addr("_ZTVNSt3__113basic_istreamIcNS_11char_traitsIcEEEE", CXX_VT_ISTREAM);
    uint64_t rtti_cursor = 0x7600009000ULL;
    for (int s = 0; s < dyn_sym_count; s++) {
        if (dyn_syms[s].defined || data_bindings[s]) continue;
        if (!strncmp(dyn_syms[s].name, "_ZTI", 4) || !strncmp(dyn_syms[s].name, "_ZTV", 4)) {
            data_bindings[s] = rtti_cursor;
            rtti_cursor += 128;
        }
    }
}

static uint64_t cxx_guest_alloc(Ctx *c, uint64_t size) {
    uint64_t save[4] = {c->x[0], c->x[1], c->x[2], c->x[30]};
    c->x[0] = 16; c->x[1] = size;
    call_indirect(c, 0x710392dce0ULL);
    uint64_t p = c->x[0];
    c->x[0] = save[0]; c->x[1] = save[1]; c->x[2] = save[2]; c->x[30] = save[3];
    return p;
}
static void cxx_guest_free(Ctx *c, uint64_t ptr) {
    if (!ptr) return;
    uint64_t save[4] = {c->x[0], c->x[1], c->x[2], c->x[30]};
    c->x[0] = ptr;
    call_indirect(c, 0x710392e590ULL);
    c->x[0] = save[0]; c->x[1] = save[1]; c->x[2] = save[2]; c->x[30] = save[3];
}
static void cxx_mutex_op(Ctx *c, const char *name, uint64_t a0, uint64_t a1) {
    uint64_t save[3] = {c->x[0], c->x[1], c->x[30]};
    c->x[0] = a0; c->x[1] = a1;
    web_mutex_import(c, name);
    c->x[0] = save[0]; c->x[1] = save[1]; c->x[30] = save[2];
}
static void cxx_short_string(uint64_t s, const char *text) {
    size_t n = strlen(text);
    for (uint64_t i = 0; i < 24; i += 8) wr64(s + i, 0);
    wr8(s, (uint8_t)(n << 1));
    for (size_t i = 0; i < n; i++) wr8(s + 1 + i, (uint8_t)text[i]);
}
static void cxx_ios_destroy(Ctx *c, uint64_t ios) {
    for (uint64_t n = rd64(ios + 0x48); n-- > 0;) {
        uint64_t fn = rd64(rd64(ios + 0x38) + n * 8);
        uint64_t save[4] = {c->x[0], c->x[1], c->x[2], c->x[30]};
        c->x[0] = 0; c->x[1] = ios; c->x[2] = rd32(rd64(ios + 0x40) + n * 4);
        if (fn) call_indirect(c, fn);
        c->x[0] = save[0]; c->x[1] = save[1]; c->x[2] = save[2]; c->x[30] = save[3];
    }
    static const uint64_t offs[4] = {0x38, 0x40, 0x58, 0x70};
    for (int i = 0; i < 4; i++) cxx_guest_free(c, rd64(ios + offs[i]));
}
static int web_cxx_indirect(Ctx *c, uint64_t address) {
    if (address < CXX_FN_BASE || address >= CXX_FN_BASE + 0x400ULL || (address & 15)) return 0;
    uint64_t off = address - CXX_FN_BASE;
    if (off < 0x100ULL) {
        unsigned slot = (unsigned)(off / 16);
        switch (slot) {
        case 0: case 1: return 1;
        case 2: return 1;
        case 3: { int ch = (uint8_t)c->x[1]; c->x[0] = (ch >= 'a' && ch <= 'z') ? (uint64_t)(ch - 0x20) : (uint64_t)ch; return 1; }
        case 4: { for (uint64_t p = c->x[1]; p < c->x[2]; p++) { uint8_t ch = rd8(p); if (ch >= 'a' && ch <= 'z') wr8(p, ch - 0x20); } c->x[0] = c->x[2]; return 1; }
        case 5: { int ch = (uint8_t)c->x[1]; c->x[0] = (ch >= 'A' && ch <= 'Z') ? (uint64_t)(ch + 0x20) : (uint64_t)ch; return 1; }
        case 6: { for (uint64_t p = c->x[1]; p < c->x[2]; p++) { uint8_t ch = rd8(p); if (ch >= 'A' && ch <= 'Z') wr8(p, ch + 0x20); } c->x[0] = c->x[2]; return 1; }
        case 7: c->x[0] = (uint8_t)c->x[1]; return 1;
        case 8: { for (uint64_t i = 0; c->x[1] + i < c->x[2]; i++) wr8(c->x[3] + i, rd8(c->x[1] + i)); c->x[0] = c->x[2]; return 1; }
        case 9: c->x[0] = (int8_t)c->x[1] >= 0 ? (uint8_t)c->x[1] : (uint8_t)c->x[2]; return 1;
        case 10: { for (uint64_t i = 0; c->x[1] + i < c->x[2]; i++) { int8_t ch = (int8_t)rd8(c->x[1] + i); wr8(c->x[4] + i, ch >= 0 ? (uint8_t)ch : (uint8_t)c->x[3]); } c->x[0] = c->x[2]; return 1; }
        }
        return 1;
    }
    if (off >= 0x100ULL && off < 0x200ULL) {
        unsigned slot = (unsigned)((off - 0x100ULL) / 16);
        switch (slot) {
        case 0: case 1: case 2: return 1;
        case 3: c->x[0] = rd8(c->x[0] + 0x10); return 1;
        case 4: c->x[0] = rd8(c->x[0] + 0x11); return 1;
        case 5: cxx_short_string(c->x[8], ""); return 1;
        case 6: cxx_short_string(c->x[8], "true"); return 1;
        case 7: cxx_short_string(c->x[8], "false"); return 1;
        }
        return 1;
    }
    if (off >= 0x200ULL && off < 0x300ULL) {
        unsigned slot = (unsigned)((off - 0x200ULL) / 16);
        uint64_t st = c->x[0];
        if (slot == 0) {
            if (st) {
                wr64(st + 0x10, 0);
                cxx_mutex_op(c, "_ZNSt3__118condition_variableD1Ev", st + 0x38, 0);
                cxx_mutex_op(c, "_ZNSt3__15mutexD1Ev", st + 0x18, 0);
            }
            return 1;
        }
        if (slot == 1 || slot == 2) {
            if (st) {
                wr64(st + 0x10, 0);
                cxx_mutex_op(c, "_ZNSt3__118condition_variableD1Ev", st + 0x38, 0);
                cxx_mutex_op(c, "_ZNSt3__15mutexD1Ev", st + 0x18, 0);
                cxx_guest_free(c, st);
            }
            return 1;
        }
        return 1;
    }
    if (off >= 0x300ULL && off < 0x400ULL) {
        unsigned slot = (unsigned)((off - 0x300ULL) / 16);
        uint64_t self = c->x[0];
        if (slot == 0) { cxx_ios_destroy(c, self + 8); c->x[0] = self; return 1; }
        if (slot == 1) { cxx_ios_destroy(c, self + 8); cxx_guest_free(c, self); return 1; }
        if (slot == 2) { uint64_t s = self + rd64(rd64(self) - 0x18); cxx_ios_destroy(c, self); c->x[0] = s; return 1; }
        if (slot == 3) { uint64_t s = self + rd64(rd64(self) - 0x18); cxx_ios_destroy(c, self); cxx_guest_free(c, s); return 1; }
        if (slot == 4) { cxx_ios_destroy(c, self + 0x10); c->x[0] = self; return 1; }
        if (slot == 5) { cxx_ios_destroy(c, self + 0x10); cxx_guest_free(c, self); return 1; }
        return 1;
    }
    return 0;
}

void guest_unimpl(uint64_t pc, uint32_t word, const char *what) { (void)word; guest_trap(what, pc); }
void guest_svc(Ctx *c, uint32_t number) { (void)c; guest_trap("unported svc", number); }
void stx_diag(Ctx *c, uint64_t a, uint64_t wanted, uint64_t seen, uint64_t value, int bits) {
    (void)c; (void)a; (void)wanted; (void)seen; (void)value; (void)bits;
}
void call_indirect(Ctx *c, uint64_t address) {
    if (web_gpu_indirect(c, address)) return;
    if (web_os_indirect(c, address)) return;
    if (web_cxx_indirect(c, address)) return;
    if (address >= 0x7400000000ULL && address < 0x7401000000ULL && !(address & 255)) {
        host_import(c, (int)((address - 0x7400000000ULL) / 256)); return;
    }
    int lo = 0, hi = fn_table_count;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (fn_table[mid].addr < address) lo = mid + 1; else hi = mid;
    }
    if (lo < fn_table_count && fn_table[lo].addr == address) { fn_table[lo].fn(c); return; }
    guest_trap("uncompiled indirect target", address);
}
void nro_catch_call(Ctx *c, void (*body)(Ctx *, uint64_t)) {
    // Normal entry is supported; actual guest throws stop in their import until
    // landing-pad unwinding exists. No exception is silently swallowed here.
    body(c, 0);
}
static void memory_copy(uint64_t dst, uint64_t src, uint64_t size) {
    unsigned char buffer[4096];
    int backwards = dst > src && dst - src < size;
    for (uint64_t n = 0; n < size;) {
        size_t step = size - n < sizeof buffer ? (size_t)(size - n) : sizeof buffer;
        uint64_t offset = backwards ? size - n - step : n;
        web_read(src + offset, buffer, step); web_write(dst + offset, buffer, step);
        n += step;
    }
}
static void memory_set(uint64_t dst, unsigned char byte, uint64_t size) {
    unsigned char buffer[4096]; memset(buffer, byte, sizeof buffer);
    for (uint64_t n = 0; n < size;) {
        size_t step = GM_PAGE_SIZE - (size_t)((dst + n) % GM_PAGE_SIZE);
        if (step > size - n) step = (size_t)(size - n);
        web_write(dst + n, buffer, step); n += step;
    }
}
GmStatus gm_os_commit(GuestMemory *, GuestAddr, size_t);
GmStatus gm_os_decommit(GuestMemory *, GuestAddr, size_t);
typedef struct { uint64_t function, arg, dso; } ExitCallback;
static ExitCallback exit_callbacks[1024];
static size_t exit_count;
static int compare_bytes(uint64_t first, uint64_t second, uint64_t size, int string) {
    for (uint64_t i = 0; i < size; i++) {
        unsigned char a = rd8(first + i), b = rd8(second + i);
        if (a != b) return (int)a - (int)b;
        if (string && !a) return 0;
    }
    return 0;
}
static size_t read_string(uint64_t address, char *out, size_t capacity) {
    for (size_t i = 0; i < capacity; i++) { out[i] = (char)rd8(address + i); if (!out[i]) return i; }
    guest_trap("host string input exceeds buffer", address); return 0;
}
static atomic_uint tls_used[64];
static uint64_t tls_destructors[64];
static _Thread_local uint64_t tls_values[64];
uint64_t web_next_prime(uint64_t number) {
    if (!number) return 0;
    if (number <= 2) return 2;
    number |= 1;
    for (;;) {
        int prime = 1;
        for (uint64_t divisor = 3; divisor <= number / divisor; divisor += 2)
            if (!(number % divisor)) { prime = 0; break; }
        if (prime) return number;
        if (number > UINT64_MAX - 2) guest_trap("next_prime overflow", number);
        number += 2;
    }
}
static void host_import_body(Ctx *c, int symbol) {
    import_calls++;
    if (startup_host_import(c, symbol)) return;
    if (symbol < 0 || symbol >= dyn_sym_count) guest_trap("invalid import index", (uint64_t)symbol);
    const char *name = dyn_syms[symbol].name;
    if (web_mutex_import(c,name)) return;
    if (web_semaphore_import(c,name)) return;
    if (web_os_import(c, name)) return;
    if (web_fs_import(c, name)) return;
    if (web_gpu_import(c, name)) return;
    if (web_string_import(c, name)) return;
    if (web_unsigned_sort_import(c, name)) return;
    if (!strcmp(name, "_ZNSt3__112__next_primeEm")) { c->x[0] = web_next_prime(c->x[0]); return; }
#define MATH_ONE(function, field) if (!strcmp(name, #function)) { errno = (int)rd32(errno_address); c->v[0].field[0] = function(c->v[0].field[0]); wr32(errno_address, (uint32_t)errno); return; }
#define MATH_TWO(function, field) if (!strcmp(name, #function)) { errno = (int)rd32(errno_address); c->v[0].field[0] = function(c->v[0].field[0], c->v[1].field[0]); wr32(errno_address, (uint32_t)errno); return; }
    MATH_ONE(log2f, f) MATH_ONE(logf, f) MATH_ONE(log10f, f) MATH_ONE(expf, f) MATH_ONE(exp2f, f)
    MATH_ONE(sinf, f) MATH_ONE(cosf, f) MATH_ONE(tanf, f) MATH_ONE(atanf, f) MATH_ONE(asinf, f) MATH_ONE(acosf, f)
    MATH_ONE(sqrtf, f) MATH_ONE(floorf, f) MATH_ONE(ceilf, f) MATH_ONE(roundf, f) MATH_ONE(truncf, f) MATH_ONE(erfcf, f)
    MATH_TWO(powf, f) MATH_TWO(atan2f, f) MATH_TWO(fmodf, f)
    MATH_ONE(log2, df) MATH_ONE(log, df) MATH_ONE(log10, df) MATH_ONE(exp, df) MATH_ONE(exp2, df)
    MATH_ONE(sin, df) MATH_ONE(cos, df) MATH_ONE(tan, df) MATH_ONE(atan, df) MATH_ONE(asin, df) MATH_ONE(acos, df)
    MATH_ONE(sqrt, df) MATH_ONE(floor, df) MATH_ONE(ceil, df) MATH_ONE(round, df) MATH_ONE(trunc, df)
    MATH_TWO(pow, df) MATH_TWO(atan2, df) MATH_TWO(fmod, df)
#undef MATH_ONE
#undef MATH_TWO
    if (!strcmp(name, "frexpf")) { int exp = 0; c->v[0].f[0] = frexpf(c->v[0].f[0], &exp); if (c->x[0]) wr32(c->x[0], (uint32_t)exp); return; }
    if (!strcmp(name, "modff")) { float ip = 0.0f; c->v[0].f[0] = modff(c->v[0].f[0], &ip); if (c->x[0]) web_write(c->x[0], &ip, 4); return; }
    if (!strcmp(name, "__nnmusl_init_dso") || !strcmp(name, "__nnmusl_fini_dso")) { c->x[0] = 0; return; }
    if (!strcmp(name, "memcpy") || !strcmp(name, "memmove")) { memory_copy(c->x[0], c->x[1], c->x[2]); return; }
    if (!strcmp(name, "memset")) { memory_set(c->x[0], (unsigned char)c->x[1], c->x[2]); return; }
    if (!strcmp(name, "memset_s")) {
        uint64_t n = c->x[3] < c->x[1] ? c->x[3] : c->x[1];
        if (c->x[0] && n) memory_set(c->x[0], (unsigned char)c->x[2], n);
        c->x[0] = 0; return;
    }
    if (!strcmp(name, "memcmp") || !strcmp(name, "strncmp") || !strcmp(name, "strcmp") || !strcmp(name, "strcoll")) {
        uint64_t size = (!strcmp(name, "strcmp") || !strcmp(name, "strcoll")) ? UINT64_MAX : c->x[2];
        c->x[0] = (uint64_t)(int64_t)compare_bytes(c->x[0], c->x[1], size, strcmp(name, "memcmp") != 0); return;
    }
    if (!strcmp(name, "strcasecmp") || !strcmp(name, "strncasecmp")) {
        uint64_t a = c->x[0], b = c->x[1], max = !strcmp(name, "strcasecmp") ? UINT64_MAX : c->x[2];
        int diff = 0;
        for (uint64_t i = 0; i < max; i++) {
            unsigned char ca = rd8(a + i), cb = rd8(b + i);
            int d = tolower(ca) - tolower(cb);
            if (d || !ca) { diff = d; break; }
        }
        c->x[0] = (uint64_t)(int64_t)diff; return;
    }
    if (!strcmp(name, "strlen")) { uint64_t size = 0; while (rd8(c->x[0] + size)) size++; c->x[0] = size; return; }
    if (!strcmp(name, "strnlen")) { uint64_t size = 0, max = c->x[1]; while (size < max && rd8(c->x[0] + size)) size++; c->x[0] = size; return; }
    if (!strcmp(name, "strcpy")) {
        uint64_t dst = c->x[0], src = c->x[1], i = 0;
        for (;; i++) { uint8_t ch = rd8(src + i); wr8(dst + i, ch); if (!ch) break; }
        c->x[0] = dst; return;
    }
    if (!strcmp(name, "strncpy")) {
        uint64_t dst = c->x[0], src = c->x[1], n = c->x[2], i = 0;
        for (; i < n; i++) { uint8_t ch = rd8(src + i); wr8(dst + i, ch); if (!ch) { for (++i; i < n; i++) wr8(dst + i, 0); break; } }
        c->x[0] = dst; return;
    }
    if (!strcmp(name, "strcat") || !strcmp(name, "strncat")) {
        uint64_t dst = c->x[0], src = c->x[1], max = !strcmp(name, "strcat") ? UINT64_MAX : c->x[2], dlen = 0, i = 0;
        while (rd8(dst + dlen)) dlen++;
        for (; i < max; i++) { uint8_t ch = rd8(src + i); if (!ch) break; wr8(dst + dlen + i, ch); }
        wr8(dst + dlen + i, 0);
        c->x[0] = dst; return;
    }
    if (!strcmp(name, "strchr") || !strcmp(name, "strrchr")) {
        uint64_t s = c->x[0], found = 0; uint8_t want = (uint8_t)c->x[1];
        int last = !strcmp(name, "strrchr");
        for (uint64_t i = 0;; i++) {
            uint8_t ch = rd8(s + i);
            if (ch == want) { found = s + i; if (!last) break; }
            if (!ch) break;
        }
        c->x[0] = found; return;
    }
    if (!strcmp(name, "memchr")) {
        uint64_t s = c->x[0], n = c->x[2], found = 0; uint8_t want = (uint8_t)c->x[1];
        for (uint64_t i = 0; i < n; i++) {
            if (rd8(s + i) == want) { found = s + i; break; }
        }
        c->x[0] = found; return;
    }
    if (!strcmp(name, "strstr")) {
        uint64_t hay = c->x[0], needle = c->x[1], nlen = 0;
        while (rd8(needle + nlen)) nlen++;
        if (!nlen) { c->x[0] = hay; return; }
        uint64_t found = 0;
        for (uint64_t i = 0; rd8(hay + i); i++) {
            if (!compare_bytes(hay + i, needle, nlen, 0)) { found = hay + i; break; }
        }
        c->x[0] = found; return;
    }
    if (!strcmp(name, "strspn") || !strcmp(name, "strcspn") || !strcmp(name, "strpbrk")) {
        uint64_t s = c->x[0], accept = c->x[1];
        uint8_t set[256] = {0};
        for (uint64_t j = 0; ; j++) { uint8_t ch = rd8(accept + j); if (!ch) break; set[ch] = 1; }
        int want = !strcmp(name, "strspn") ? 1 : 0;
        uint64_t i = 0;
        for (;; i++) {
            uint8_t ch = rd8(s + i);
            if (!ch) break;
            if (!strcmp(name, "strpbrk")) { if (set[ch]) { c->x[0] = s + i; return; } }
            else if (set[ch] != want) break;
        }
        c->x[0] = !strcmp(name, "strpbrk") ? 0 : i; return;
    }
    if (!strcmp(name, "strerror")) {
        uint64_t msg = errno_address + 224;
        web_write(msg, "error", 6);
        c->x[0] = msg; return;
    }
    if (!strcmp(name, "getenv")) { c->x[0] = 0; return; }
    if (!strcmp(name, "__errno_location")) { c->x[0] = errno_address; return; }
    if (!strcmp(name, "gettimeofday")) {
        uint64_t us = (uint64_t)(emscripten_date_now() * 1000.0);
        if (c->x[0]) { wr64(c->x[0], us / 1000000); wr64(c->x[0] + 8, us % 1000000); }
        c->x[0] = 0; return;
    }
    if (!strcmp(name, "_ZN2nn2os16GetCurrentThreadEv")) { c->x[0] = main_thread; return; }
    if (!strcmp(name, "_ZN2nn2os25GetCurrentProcessorNumberEv") || !strcmp(name, "_ZN2nn2os20GetCurrentCoreNumberEv")) { c->x[0] = (uint64_t)main_core; return; }
    if (!strcmp(name, "_ZN2nn2os26GetThreadAvailableCoreMaskEv")) { c->x[0] = 7; return; }
    if (!strcmp(name, "_ZN2nn2os17GetThreadCoreMaskEPiPmPKNS0_10ThreadTypeE")) {
        if (c->x[2] && c->x[2] != main_thread) guest_trap("unknown ThreadType", c->x[2]);
        if (c->x[0]) wr32(c->x[0], (uint32_t)main_core); if (c->x[1]) wr64(c->x[1], main_core_mask); return;
    }
    if (!strcmp(name, "_ZN2nn2os17SetThreadCoreMaskEPNS0_10ThreadTypeEim")) {
        if (c->x[0] && c->x[0] != main_thread) guest_trap("unknown ThreadType", c->x[0]);
        int ideal = (int)c->x[1]; uint64_t mask = c->x[2];
        if (ideal == -2) { main_core = 0; main_core_mask = 1; }
        else if (ideal == -3) main_core_mask = mask;
        else { main_core_mask = mask; if (ideal >= 0) main_core = ideal; else if (mask && !(mask & (1ULL << main_core))) main_core = __builtin_ctzll(mask); }
        return;
    }
    if (!strcmp(name, "_ZN2nn2os19GetCurrentStackInfoEPmS1_")) {
        if (c->x[0]) wr64(c->x[0], 0x7300100000ULL); if (c->x[1]) wr64(c->x[1], 1048576); return;
    }
    if (!strcmp(name, "_ZN2nn2os17GetThreadPriorityEPKNS0_10ThreadTypeE") || !strcmp(name, "_ZN2nn2os24GetThreadCurrentPriorityEPKNS0_10ThreadTypeE")) {
        if (c->x[0] != main_thread) guest_trap("unknown ThreadType", c->x[0]); c->x[0] = (uint64_t)(int64_t)main_priority; return;
    }
    if (!strcmp(name, "_ZN2nn2os20ChangeThreadPriorityEPNS0_10ThreadTypeEi")) {
        if (c->x[0] != main_thread) guest_trap("unknown ThreadType", c->x[0]); int old = main_priority; main_priority = (int)c->x[1]; c->x[0] = (uint64_t)(int64_t)old; return;
    }
    if (!strcmp(name, "_ZN2nn2os11GetThreadIdEPKNS0_10ThreadTypeE")) {
        if (c->x[0] != main_thread) guest_trap("unknown ThreadType", c->x[0]); c->x[0] = 1; return;
    }
    if (!strcmp(name, "atoi") || !strcmp(name, "atol")) {
        char text[256]; read_string(c->x[0], text, sizeof text);
        c->x[0] = !strcmp(name, "atoi") ? (uint64_t)(int64_t)atoi(text) : (uint64_t)strtoll(text, NULL, 10); return;
    }
    if (!strcmp(name, "strtol") || !strcmp(name, "strtoll") || !strcmp(name, "strtoul") || !strcmp(name, "strtoull")) {
        char text[4096], *end;
        uint64_t input = c->x[0], output = c->x[1];
        read_string(input, text, sizeof text); errno = (int)rd32(errno_address);
        uint64_t value = (!strcmp(name, "strtoull") || !strcmp(name, "strtoul")) ? (uint64_t)strtoull(text, &end, (int)c->x[2]) : (uint64_t)strtoll(text, &end, (int)c->x[2]);
        wr32(errno_address, (uint32_t)errno);
        if (output) wr64(output, input + (uint64_t)(end - text));
        c->x[0] = value; return;
    }
    if (!strcmp(name, "strtof")) {
        char text[4096], *end;
        uint64_t input = c->x[0], output = c->x[1];
        read_string(input, text, sizeof text); errno = (int)rd32(errno_address);
        float value = strtof(text, &end);
        wr32(errno_address, (uint32_t)errno);
        if (output) wr64(output, input + (uint64_t)(end - text));
        c->v[0].f[0] = value; return;
    }
    if (!strcmp(name, "atof")) {
        char text[4096]; read_string(c->x[0], text, sizeof text);
        c->v[0].df[0] = strtod(text, NULL); return;
    }
    if (!strcmp(name, "tolower")) { c->x[0] = (uint64_t)(int64_t)tolower((int)c->x[0]); return; }
    if (!strcmp(name, "toupper")) { c->x[0] = (uint64_t)(int64_t)toupper((int)c->x[0]); return; }
    if (!strcmp(name, "isalnum")) { c->x[0] = isalnum((int)c->x[0] & 255) != 0; return; }
    if (!strcmp(name, "iscntrl")) { c->x[0] = iscntrl((int)c->x[0] & 255) != 0; return; }
    if (!strcmp(name, "isxdigit")) { c->x[0] = isxdigit((int)c->x[0] & 255) != 0; return; }
    if (!strcmp(name, "ispunct")) { c->x[0] = ispunct((int)c->x[0] & 255) != 0; return; }
    if (!strcmp(name, "isspace")) { c->x[0] = isspace((int)c->x[0] & 255) != 0; return; }
    if (!strcmp(name, "sscanf")) {
        char input[4096], fmt[1024];
        read_string(c->x[0], input, sizeof input);
        read_string(c->x[1], fmt, sizeof fmt);
        uint64_t args[16];
        for (int i = 0; i < 6; i++) args[i] = c->x[2 + i];
        for (int i = 6; i < 16; i++) args[i] = rd64(c->x[31] + (uint64_t)(i - 6) * 8);
        int arg_idx = 0, matches = 0;
        const char *s = input, *p = fmt;
        while (*p) {
            if (isspace((unsigned char)*p)) { while (isspace((unsigned char)*p)) p++; while (isspace((unsigned char)*s)) s++; continue; }
            if (*p != '%' || p[1] == '%') {
                if (*p == '%') p++;
                if (*s != *p) break;
                s++; p++; continue;
            }
            p++;
            int suppress = (*p == '*') ? (p++, 1) : 0;
            int width = 0; while (isdigit((unsigned char)*p)) width = width * 10 + (*p++ - '0');
            int size = 0;
            while (*p == 'h' || *p == 'l' || *p == 'j' || *p == 'z' || *p == 't') {
                if (*p == 'h') size = size == -1 ? -2 : -1;
                else size = size >= 1 ? 2 : 1;
                p++;
            }
            char conv = *p ? *p++ : 0;
            if (!conv) break;
            if (conv == 'n') {
                if (!suppress && arg_idx < 16) wr32(args[arg_idx++], (uint32_t)(s - input));
                continue;
            }
            if (conv != 'c') while (isspace((unsigned char)*s)) s++;
            if (!*s) { if (!matches) matches = -1; break; }
            uint64_t dst = (!suppress && arg_idx < 16) ? args[arg_idx++] : 0;
            if (conv == 'd' || conv == 'i' || conv == 'u' || conv == 'x' || conv == 'X' || conv == 'o' || conv == 'p') {
                char *end = NULL;
                int base = (conv == 'd' || conv == 'u') ? 10 : conv == 'o' ? 8 : conv == 'i' ? 0 : 16;
                uint64_t val = (conv == 'd' || conv == 'i') ? (uint64_t)strtoll(s, &end, base) : (uint64_t)strtoull(s, &end, base);
                if (end == s) break;
                s = end;
                if (dst) {
                    if (conv == 'p' || size >= 1) wr64(dst, val);
                    else if (size == -2) wr8(dst, (uint8_t)val);
                    else if (size == -1) wr16(dst, (uint16_t)val);
                    else wr32(dst, (uint32_t)val);
                    matches++;
                }
            } else if (conv == 'f' || conv == 'e' || conv == 'g' || conv == 'E' || conv == 'G') {
                char *end = NULL;
                double d = strtod(s, &end);
                if (end == s) break;
                s = end;
                if (dst) {
                    if (size >= 1) web_write(dst, &d, 8);
                    else { float f = (float)d; web_write(dst, &f, 4); }
                    matches++;
                }
            } else if (conv == 's') {
                int maxw = width ? width : 4095, k = 0;
                while (s[k] && !isspace((unsigned char)s[k]) && k < maxw) { if (dst) wr8(dst + (uint64_t)k, (uint8_t)s[k]); k++; }
                if (!k) break;
                if (dst) { wr8(dst + (uint64_t)k, 0); matches++; }
                s += k;
            } else if (conv == 'c') {
                int n = width ? width : 1;
                for (int k = 0; k < n && *s; k++, s++) if (dst) wr8(dst + (uint64_t)k, (uint8_t)*s);
                if (dst) matches++;
            } else break;
        }
        c->x[0] = (uint64_t)(int64_t)matches; return;
    }
    if (!strcmp(name, "bsearch")) {
        uint64_t key = c->x[0], base = c->x[1], nmemb = c->x[2], size = c->x[3], compar = c->x[4], lr = c->x[30];
        uint64_t lo = 0, hi = nmemb, found = 0;
        while (lo < hi) {
            uint64_t mid = lo + (hi - lo) / 2, elem = base + mid * size;
            c->x[0] = key; c->x[1] = elem;
            call_indirect(c, compar);
            int cmp = (int32_t)c->x[0];
            if (cmp < 0) hi = mid;
            else if (cmp > 0) lo = mid + 1;
            else { found = elem; break; }
        }
        c->x[30] = lr; c->x[0] = found; return;
    }
    if (!strcmp(name, "qsort")) {
        uint64_t base = c->x[0], nmemb = c->x[1], size = c->x[2], compar = c->x[3], lr = c->x[30];
        if (nmemb >= 2 && size > 0 && compar) {
            unsigned char *tmp = malloc((size_t)size);
            if (!tmp) guest_trap("qsort allocation failed", size);
            for (uint64_t gap = nmemb / 2; gap > 0; gap /= 2) {
                for (uint64_t i = gap; i < nmemb; i++) {
                    uint64_t j = i;
                    while (j >= gap) {
                        uint64_t a = base + (j - gap) * size, b = base + j * size;
                        c->x[0] = a; c->x[1] = b;
                        call_indirect(c, compar);
                        if ((int32_t)c->x[0] <= 0) break;
                        web_read(a, tmp, (size_t)size);
                        memory_copy(a, b, size);
                        web_write(b, tmp, (size_t)size);
                        j -= gap;
                    }
                }
            }
            free(tmp);
        }
        c->x[30] = lr; return;
    }
    if (!strcmp(name, "_ZNSt3__113random_deviceC1ERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEE")) {
        uint64_t self = c->x[0]; uint32_t x = 5489;
        wr32(self, x);
        for (uint32_t i = 1; i < 624; i++) { x = (x ^ (x >> 30)) * 1812433253u + i; wr32(self + 4ULL * i, x); }
        wr64(self + 0x9c0, 0); return;
    }
    if (!strcmp(name, "_ZNSt3__113random_deviceclEv")) {
        uint64_t self = c->x[0], i = rd64(self + 0x9c0) % 624, j = (i + 1) % 624, k = (i + 397) % 624;
        uint32_t xj = rd32(self + 4 * j), y = (rd32(self + 4 * i) & 0x80000000u) | (xj & 0x7fffffffu);
        uint32_t v = rd32(self + 4 * k) ^ (y >> 1) ^ ((xj & 1) * 0x9908b0dfu);
        wr32(self + 4 * i, v); wr64(self + 0x9c0, j);
        v ^= v >> 11; v ^= (v << 7) & 0x9d2c5680u; v ^= (v << 15) & 0xefc60000u; v ^= v >> 18;
        c->x[0] = v; return;
    }
    if (!strcmp(name, "_ZNSt3__113random_deviceD1Ev")) return;
    if (!strcmp(name, "rand") || !strcmp(name, "srand")) {
        static uint64_t rng = 1;
        if (!strcmp(name, "srand")) { rng = (uint32_t)c->x[0] ? (uint32_t)c->x[0] : 1; return; }
        uint64_t z = (rng += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        c->x[0] = (z ^ (z >> 31)) & 0x7fffffffULL; return;
    }
    if (!strcmp(name, "clock")) { c->x[0] = (uint64_t)(emscripten_get_now() * 1000.0); return; }
    if (!strcmp(name, "time")) {
        uint64_t sec = (uint64_t)(emscripten_date_now() / 1000.0);
        if (c->x[0]) wr64(c->x[0], sec);
        c->x[0] = sec; return;
    }
    if (!strcmp(name, "atexit")) {
        if (exit_count < 1024) exit_callbacks[exit_count++] = (ExitCallback){c->x[0], 0, 0};
        c->x[0] = 0; return;
    }
    if (!strcmp(name, "fflush")) { c->x[0] = 0; return; }
    if (!strcmp(name, "localeconv")) {
        uint64_t dot = errno_address + 120, empty = errno_address + 121, lc = errno_address + 128;
        wr8(dot, '.'); wr8(empty, 0);
        wr64(lc, dot);
        for (unsigned i = 1; i < 10; i++) wr64(lc + i * 8, empty);
        for (unsigned i = 0; i < 16; i++) wr8(lc + 80 + i, 127);
        c->x[0] = lc; return;
    }
    if (!strcmp(name, "ffsl") || !strcmp(name, "ffsll")) { c->x[0] = (uint64_t)__builtin_ffsll((long long)c->x[0]); return; }
    if (!strcmp(name, "_ZN2nn2os21AllocateAddressRegionEPmm")) {
        uint64_t size = c->x[1], address = next_region;
        if (!size || size > UINT64_MAX - GM_PAGE_SIZE || size > UINT64_MAX - address - 65536) { c->x[0] = 0x1015; return; }
        size = (size + GM_PAGE_SIZE - 1) & ~(uint64_t)(GM_PAGE_SIZE - 1);
        GmStatus status = gm_reserve(elf_memory(), address, size);
        wr64(c->x[0], status == GM_OK ? address : 0);
        if (status == GM_OK) next_region += size + 65536;
        c->x[0] = status == GM_OK ? 0 : 0x1015; return;
    }
    if (!strcmp(name, "_ZN2nn2os19AllocateMemoryPagesEmm")) {
        if (c->x[1] > SIZE_MAX) { c->x[0] = 0x1015; return; }
        GmStatus status = gm_os_commit(elf_memory(), c->x[0], (size_t)c->x[1]);
        c->x[0] = status == GM_OK ? 0 : 0x1015; return;
    }
    if (!strcmp(name, "_ZN2nn2os15FreeMemoryPagesEmm")) {
        if (c->x[1] > SIZE_MAX) { c->x[0] = 0x1015; return; }
        GmStatus status = gm_os_decommit(elf_memory(), c->x[0], (size_t)c->x[1]);
        c->x[0] = status == GM_OK ? 0 : 0x1015; return;
    }
    if (!strcmp(name, "__cxa_atexit")) {
        if (exit_count == 1024) guest_trap("atexit capacity", c->x[30]);
        exit_callbacks[exit_count++] = (ExitCallback){c->x[0], c->x[1], c->x[2]}; c->x[0] = 0; return;
    }
    if (!strcmp(name, "__cxa_guard_acquire")) {
        uint64_t guard = c->x[0];
        if (rd8(guard) & 1) c->x[0] = 0;
        else { if (rd8(guard + 1)) guest_trap("recursive static initialisation", guard); wr8(guard + 1, 1); c->x[0] = 1; }
        return;
    }
    if (!strcmp(name, "__cxa_guard_release")) { wr8(c->x[0], 1); wr8(c->x[0] + 1, 0); return; }
    if (!strcmp(name, "__cxa_guard_abort")) { wr8(c->x[0] + 1, 0); return; }
    if (!strcmp(name, "_ZN2nn2os13GetSystemTickEv") || !strcmp(name, "nnosGetSystemTick")) { c->x[0] = (uint64_t)(emscripten_get_now() * 19200.0); return; }
    if (!strcmp(name, "_ZN2nn2os22GetSystemTickFrequencyEv") || !strcmp(name, "nnosGetSystemTickFrequency")) { c->x[0] = 19200000; return; }
    if (!strcmp(name, "_ZN2nn2os18SdkAllocateTlsSlotEPNS0_7TlsSlotEPFvmE") || !strcmp(name, "_ZN2nn2os15AllocateTlsSlotEPNS0_7TlsSlotEPFvmE")) {
        for (unsigned i = 0; i < 64; i++) {
            unsigned expected = 0;
            if (atomic_compare_exchange_strong(&tls_used[i], &expected, 2)) {
                tls_destructors[i] = c->x[1];
                atomic_store(&tls_used[i], 1);
                wr32(c->x[0], i); c->x[0] = 0; return;
            }
        }
        guest_trap("TLS slots exhausted", c->x[30]);
    }
    if (!strcmp(name, "_ZN2nn2os11FreeTlsSlotENS0_7TlsSlotE")) {
        if (c->x[0] < 64) { tls_values[c->x[0]] = 0; atomic_store(&tls_used[c->x[0]], 0); } return;
    }
    if (!strcmp(name, "_ZN2nn2os11SetTlsValueENS0_7TlsSlotEm")) {
        if (c->x[0] >= 64 || atomic_load(&tls_used[c->x[0]]) != 1) guest_trap("invalid TLS slot", c->x[0]); tls_values[c->x[0]] = c->x[1]; return;
    }
    if (!strcmp(name, "_ZN2nn2os11GetTlsValueENS0_7TlsSlotE")) {
        if (c->x[0] >= 64 || atomic_load(&tls_used[c->x[0]]) != 1) guest_trap("invalid TLS slot", c->x[0]); c->x[0] = tls_values[c->x[0]]; return;
    }
    if (!strcmp(name, "_ZNSt3__16chrono12steady_clock3nowEv")) {
        struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
        c->x[0] = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec; return;
    }
    if (!strcmp(name, "_ZNSt3__16chrono12system_clock3nowEv")) {
        struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
        c->x[0] = (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL; return;
    }
    if (!strcmp(name, "_ZNSt3__119__shared_weak_count14__release_weakEv")) {
        uint64_t self = c->x[0];
        if (!self) return;
        int64_t weak = (int64_t)rd64(self + 16);
        if (weak != 0) {
            weak--;
            wr64(self + 16, (uint64_t)weak);
            if (weak != -1) return;
        }
        uint64_t saved_lr = c->x[30];
        uint64_t fn = rd64(rd64(self) + 32);
        if (fn) call_indirect(c, fn);
        c->x[30] = saved_lr;
        return;
    }
    if (!strcmp(name, "_ZNSt3__119__shared_weak_countD2Ev") || !strcmp(name, "_ZNSt3__114__shared_countD2Ev") ||
        !strcmp(name, "_ZNSt9exceptionD2Ev") || !strcmp(name, "_ZNSt11logic_errorD2Ev") ||
        !strcmp(name, "_ZNSt12out_of_rangeD2Ev") || !strcmp(name, "_ZNSt16invalid_argumentD2Ev") ||
        !strcmp(name, "_ZNSt8bad_castD2Ev") || !strcmp(name, "_ZNSt8bad_castC2Ev") ||
        !strcmp(name, "_ZNSt9bad_allocC1Ev") || !strcmp(name, "_ZNSt11logic_errorC2EPKc") ||
        !strcmp(name, "_ZNSt11logic_errorC2ERKS_") ||
        !strcmp(name, "_ZNSt11logic_errorC2ERKNSt3__112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEE")) {
        return;
    }
    if (!strcmp(name, "_ZNSt3__119__shared_weak_count4lockEv")) {
        int64_t owners = c->x[0] ? (int64_t)rd64(c->x[0] + 8) : -1;
        if (owners == -1) { c->x[0] = 0; return; }
        wr64(c->x[0] + 8, (uint64_t)(owners + 1)); return;
    }
    if (!strcmp(name, "_ZNSt13exception_ptrC1ERKS_")) {
        wr64(c->x[0], c->x[1] ? rd64(c->x[1]) : 0);
        return;
    }
    if (!strcmp(name, "_ZNSt13exception_ptrD1Ev")) {
        if (c->x[0]) wr64(c->x[0], 0);
        return;
    }
    if (!strcmp(name, "_ZNSt3__115future_categoryEv")) { c->x[0] = CXX_FUTURE_CAT; return; }
    if (!strcmp(name, "_ZNSt3__112future_errorC1ENS_10error_codeE") || !strcmp(name, "_ZNSt3__112future_errorC2ENS_10error_codeE")) {
        if (c->x[0]) { wr64(c->x[0], 0); wr64(c->x[0] + 8, 0); wr64(c->x[0] + 16, (uint32_t)c->x[1]); wr64(c->x[0] + 24, c->x[2]); }
        return;
    }
    if (!strcmp(name, "_ZNSt3__17promiseIvEC1Ev") || !strcmp(name, "_ZNSt3__17promiseIvEC2Ev")) {
        uint64_t self = c->x[0], st = cxx_guest_alloc(c, 0x48);
        if (st) {
            for (uint64_t o = 0; o < 0x48; o += 8) wr64(st + o, 0);
            wr64(st, CXX_VT_ASSOC + 16);
            wr16(st + 0x18, 1);
            wr8(st + 0x38, 1);
        }
        wr64(self, st);
        c->x[0] = self; return;
    }
    if (!strcmp(name, "_ZNSt3__17promiseIvE10get_futureEv")) {
        uint64_t st = rd64(c->x[0]), out = c->x[8];
        if (st) {
            cxx_mutex_op(c, "_ZNSt3__15mutex4lockEv", st + 0x18, 0);
            wr64(st + 8, rd64(st + 8) + 1);
            wr32(st + 0x40, rd32(st + 0x40) | 2u);
            cxx_mutex_op(c, "_ZNSt3__15mutex6unlockEv", st + 0x18, 0);
        }
        if (out) wr64(out, st);
        return;
    }
    if (!strcmp(name, "_ZNSt3__17promiseIvE9set_valueEv")) {
        uint64_t st = rd64(c->x[0]);
        if (st) {
            cxx_mutex_op(c, "_ZNSt3__15mutex4lockEv", st + 0x18, 0);
            wr32(st + 0x40, rd32(st + 0x40) | 1u | 4u);
            cxx_mutex_op(c, "_ZNSt3__118condition_variable10notify_allEv", st + 0x38, 0);
            cxx_mutex_op(c, "_ZNSt3__15mutex6unlockEv", st + 0x18, 0);
        }
        return;
    }
    if (!strcmp(name, "_ZNSt3__17promiseIvED1Ev") || !strcmp(name, "_ZNSt3__17promiseIvED2Ev") ||
        !strcmp(name, "_ZNSt3__16futureIvED1Ev") || !strcmp(name, "_ZNSt3__16futureIvED2Ev")) {
        uint64_t self = c->x[0], st = self ? rd64(self) : 0;
        if (st) {
            int64_t next = (int64_t)rd64(st + 8) - 1;
            wr64(st + 8, (uint64_t)next);
            if (next == -1) {
                uint64_t save[2] = {c->x[0], c->x[30]};
                c->x[0] = st;
                uint64_t fn = rd64(rd64(st) + 16);
                if (fn) call_indirect(c, fn);
                c->x[0] = save[0]; c->x[30] = save[1];
            }
        }
        c->x[0] = self; return;
    }
    if (!strcmp(name, "_ZNSt3__117__assoc_sub_state10__sub_waitERNS_11unique_lockINS_5mutexEEE")) {
        uint64_t st = c->x[0], lk = c->x[1];
        uint32_t state = rd32(st + 0x40);
        if (state & 4u) return;
        if (state & 8u) {
            wr32(st + 0x40, state & ~8u);
            cxx_mutex_op(c, "_ZNSt3__15mutex6unlockEv", rd64(lk), 0);
            wr8(lk + 8, 0);
            uint64_t save_lr = c->x[30], fn = rd64(rd64(st) + 24);
            c->x[0] = st;
            if (fn) call_indirect(c, fn);
            c->x[30] = save_lr;
            return;
        }
        while (!(rd32(st + 0x40) & 4u))
            cxx_mutex_op(c, "_ZNSt3__118condition_variable4waitERNS_11unique_lockINS_5mutexEEE", st + 0x38, lk);
        return;
    }
    if (!strcmp(name, "_ZNSt3__117__assoc_sub_state16__on_zero_sharedEv")) {
        uint64_t st = c->x[0];
        if (st) {
            uint64_t save_lr = c->x[30], fn = rd64(rd64(st) + 8);
            if (fn) call_indirect(c, fn);
            c->x[30] = save_lr;
        }
        return;
    }
    if (!strcmp(name, "_ZNSt3__117__assoc_sub_state9__executeEv")) return;
    if (!strcmp(name, "_ZN2nn3err18ErrorResultVariantC1Ev")) { wr32(c->x[0], 0); return; }
    if (!strcmp(name, "_ZN2nn3err18ErrorResultVariantaSERKNS_6ResultE") || !strcmp(name, "_ZN2nn3err18ErrorResultVariantC1ERKNS_6ResultE")) {
        wr32(c->x[0], 1); wr32(c->x[0] + 4, rd32(c->x[1])); return;
    }
    if (!strcmp(name, "_ZN2nn3err18ErrorResultVariantaSERKNS0_9ErrorCodeE") || !strcmp(name, "_ZN2nn3err18ErrorResultVariantC1ERKNS0_9ErrorCodeE")) {
        wr32(c->x[0], 2); wr64(c->x[0] + 4, rd64(c->x[1])); return;
    }
    if (!strcmp(name, "_ZN2nn3ldn15MakeIpv4AddressEhhhh")) {
        c->x[0] = ((c->x[0] & 255) << 24) | ((c->x[1] & 255) << 16) | ((c->x[2] & 255) << 8) | (c->x[3] & 255); return;
    }
    if (!strcmp(name, "_ZN2nn2os11GetHostArgcEv")) { c->x[0] = 0; return; }
    if (!strcmp(name, "_ZN2nn2os11GetHostArgvEv")) { wr64(errno_address + 16, 0); wr64(errno_address + 24, 0); c->x[0] = errno_address + 16; return; }
    if (!strcmp(name, "_ZNSt3__16localeC1Ev") || !strcmp(name, "_ZNSt3__16localeC1ERKS0_")) { wr64(c->x[0], errno_address + 64); return; }
    if (!strcmp(name, "_ZNSt3__16locale7classicEv")) { wr64(errno_address + 32, errno_address + 64); c->x[0] = errno_address + 32; return; }
    if (!strcmp(name, "_ZNKSt3__16localeeqERKS0_")) { c->x[0] = 1; return; }
    if (!strcmp(name, "_ZNSt3__16localeD1Ev")) return;
    if (!strcmp(name, "_ZNKSt3__16locale9use_facetERNS0_2idE")) {
        c->x[0] = (c->x[1] == CXX_ID_NUMPUNCT) ? CXX_NUMPUNCT_OBJ : CXX_CTYPE_OBJ;
        return;
    }
    if (!strcmp(name, "_ZNKSt3__16locale9has_facetERNS0_2idE")) {
        c->x[0] = (c->x[1] == CXX_ID_CTYPE || c->x[1] == CXX_ID_NUMPUNCT);
        return;
    }
    if (!strcmp(name, "_ZNSt3__115basic_streambufIcNS_11char_traitsIcEEEC2Ev")) {
        uint64_t sb = c->x[0];
        wr64(sb + 8, errno_address + 64);
        for (uint64_t off = 0x10; off <= 0x38; off += 8) wr64(sb + off, 0);
        return;
    }
    if (!strcmp(name, "_ZNSt3__115basic_streambufIcNS_11char_traitsIcEEED2Ev") ||
        !strcmp(name, "_ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE6setbufEPcl")) return;
    if (!strcmp(name, "_ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE4syncEv") ||
        !strcmp(name, "_ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE9showmanycEv") ||
        !strcmp(name, "_ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE5imbueERKNS_6localeE")) { c->x[0] = 0; return; }
    if (!strcmp(name, "_ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE9underflowEv") ||
        !strcmp(name, "_ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE9pbackfailEi") ||
        !strcmp(name, "_ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE8overflowEi")) { c->x[0] = 0xffffffffu; return; }
    if (!strcmp(name, "_ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE5uflowEv")) {
        uint64_t sb = c->x[0], save_lr = c->x[30];
        call_indirect(c, rd64(rd64(sb) + 0x48));
        c->x[30] = save_lr;
        if ((uint32_t)c->x[0] == 0xffffffffu) return;
        uint64_t g = rd64(sb + 0x18);
        wr64(sb + 0x18, g + 1);
        c->x[0] = rd8(g); return;
    }
    if (!strcmp(name, "_ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE6xsgetnEPcl")) {
        uint64_t sb = c->x[0], s = c->x[1], save_lr = c->x[30];
        int64_t n = (int64_t)c->x[2], done = 0;
        while (done < n) {
            uint64_t g = rd64(sb + 0x18), e = rd64(sb + 0x20);
            if (g < e) {
                int64_t k = (int64_t)(e - g) < n - done ? (int64_t)(e - g) : n - done;
                memory_copy(s + (uint64_t)done, g, (uint64_t)k);
                wr64(sb + 0x18, g + (uint64_t)k);
                done += k;
            } else {
                c->x[0] = sb;
                call_indirect(c, rd64(rd64(sb) + 0x50));
                if ((uint32_t)c->x[0] == 0xffffffffu) break;
                wr8(s + (uint64_t)done++, (uint8_t)c->x[0]);
            }
        }
        c->x[30] = save_lr;
        c->x[0] = (uint64_t)done; return;
    }
    if (!strcmp(name, "_ZNSt3__115basic_streambufIcNS_11char_traitsIcEEE6xsputnEPKcl")) {
        uint64_t sb = c->x[0], src = c->x[1], save_lr = c->x[30];
        int64_t n = (int64_t)c->x[2], done = 0;
        char *copy = malloc(n > 0 ? (size_t)n : 1);
        if (n > 0 && copy) web_read(src, copy, (size_t)n);
        while (copy && done < n) {
            uint64_t p = rd64(sb + 0x30), e = rd64(sb + 0x38);
            if (p < e) {
                int64_t k = (int64_t)(e - p) < n - done ? (int64_t)(e - p) : n - done;
                web_write(p, copy + done, (size_t)k);
                wr64(sb + 0x30, p + (uint64_t)k);
                done += k;
            } else {
                c->x[0] = sb; c->x[1] = (uint8_t)copy[done];
                call_indirect(c, rd64(rd64(sb) + 0x68));
                if ((uint32_t)c->x[0] == 0xffffffffu) break;
                done++;
            }
        }
        free(copy);
        c->x[30] = save_lr;
        c->x[0] = (uint64_t)done; return;
    }
    if (!strcmp(name, "_ZNSt3__18ios_base4initEPv")) {
        uint64_t ios = c->x[0], sb = c->x[1];
        for (uint64_t off = 8; off < 0x88; off += 8) wr64(ios + off, 0);
        wr32(ios + 8, 0x1002u);
        wr64(ios + 0x10, 6);
        wr32(ios + 0x20, sb ? 0 : 1u);
        wr64(ios + 0x28, sb);
        wr64(ios + 0x30, errno_address + 64);
        return;
    }
    if (!strcmp(name, "_ZNSt3__18ios_base5clearEj")) {
        uint64_t ios = c->x[0];
        uint32_t state = rd64(ios + 0x28) ? (uint32_t)c->x[1] : ((uint32_t)c->x[1] | 1u);
        wr32(ios + 0x20, state);
        return;
    }
    if (!strcmp(name, "_ZNKSt3__18ios_base6getlocEv")) {
        wr64(c->x[8], errno_address + 64);
        c->x[0] = c->x[8]; return;
    }
    if (!strcmp(name, "_ZNSt3__19basic_iosIcNS_11char_traitsIcEEED2Ev")) {
        uint64_t self = c->x[0];
        cxx_ios_destroy(c, self);
        c->x[0] = self; return;
    }
    if (!strcmp(name, "_ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEE6sentryC1ERS3_")) {
        uint64_t s = c->x[0], os = c->x[1], ios = os + rd64(rd64(os) - 0x18);
        wr64(s, 0); wr64(s + 8, os);
        wr8(s, rd32(ios + 0x20) == 0);
        c->x[0] = s; return;
    }
    if (!strcmp(name, "_ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEE6sentryD1Ev")) return;
    if (!strcmp(name, "_ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEED2Ev") ||
        !strcmp(name, "_ZNSt3__113basic_istreamIcNS_11char_traitsIcEEED2Ev")) return;
    if (!strcmp(name, "_ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEED1Ev")) {
        uint64_t self = c->x[0]; cxx_ios_destroy(c, self + 8); c->x[0] = self; return;
    }
    if (!strcmp(name, "_ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEED0Ev")) {
        uint64_t self = c->x[0]; cxx_ios_destroy(c, self + 8); cxx_guest_free(c, self); return;
    }
    if (!strcmp(name, "_ZNSt3__113basic_istreamIcNS_11char_traitsIcEEED1Ev")) {
        uint64_t self = c->x[0]; cxx_ios_destroy(c, self + 0x10); c->x[0] = self; return;
    }
    if (!strcmp(name, "_ZNSt3__113basic_istreamIcNS_11char_traitsIcEEED0Ev")) {
        uint64_t self = c->x[0]; cxx_ios_destroy(c, self + 0x10); cxx_guest_free(c, self); return;
    }
    if (!strcmp(name, "_ZTv0_n24_NSt3__113basic_ostreamIcNS_11char_traitsIcEEED1Ev") ||
        !strcmp(name, "_ZTv0_n24_NSt3__113basic_istreamIcNS_11char_traitsIcEEED1Ev")) {
        uint64_t ios = c->x[0], self = ios + rd64(rd64(ios) - 0x18);
        cxx_ios_destroy(c, ios); c->x[0] = self; return;
    }
    if (!strcmp(name, "_ZTv0_n24_NSt3__113basic_ostreamIcNS_11char_traitsIcEEED0Ev") ||
        !strcmp(name, "_ZTv0_n24_NSt3__113basic_istreamIcNS_11char_traitsIcEEED0Ev")) {
        uint64_t ios = c->x[0], self = ios + rd64(rd64(ios) - 0x18);
        cxx_ios_destroy(c, ios); cxx_guest_free(c, self); return;
    }
    if (!strcmp(name, "_ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEElsEi") ||
        !strcmp(name, "_ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEElsEm")) {
        int is_signed = !strcmp(name, "_ZNSt3__113basic_ostreamIcNS_11char_traitsIcEEElsEi");
        uint64_t os = c->x[0], ios = os + rd64(rd64(os) - 0x18), sb = rd64(ios + 0x28), save_lr = c->x[30];
        uint32_t flags = rd32(ios + 8);
        uint64_t val = is_signed ? ((flags & 0x48u) ? (uint64_t)(uint32_t)c->x[1] : (uint64_t)(int64_t)(int32_t)c->x[1]) : c->x[1];
        char text[80], buf[160];
        if (flags & 8u) snprintf(text, sizeof text, (flags & 0x4000u) ? "%s%llX" : "%s%llx", (flags & 0x200u) ? ((flags & 0x4000u) ? "0X" : "0x") : "", (unsigned long long)val);
        else if (flags & 0x40u) snprintf(text, sizeof text, "%s%llo", ((flags & 0x200u) && val) ? "0" : "", (unsigned long long)val);
        else if (is_signed) snprintf(text, sizeof text, (flags & 0x800u) ? "%+lld" : "%lld", (long long)val);
        else snprintf(text, sizeof text, "%llu", (unsigned long long)val);
        if (rd32(ios + 0x20) == 0 && sb) {
            int len = (int)strlen(text), width = (int)rd64(ios + 0x18), fill = (int)rd32(ios + 0x90);
            if (fill <= 0 || fill > 0xff) fill = ' ';
            if (width > (int)sizeof buf - 1) width = (int)sizeof buf - 1;
            int pad = width > len ? width - len : 0, left = (flags & 0x20u) != 0, prefix = 0;
            memset(buf, fill, sizeof buf);
            if ((flags & 0xb0u) == 0x10u && pad) {
                if (text[0] == '-' || text[0] == '+') prefix = 1;
                else if (len >= 2 && text[0] == '0' && (text[1] | 0x20) == 'x') prefix = 2;
            }
            if (prefix) { memcpy(buf, text, (size_t)prefix); memcpy(buf + prefix + pad, text + prefix, (size_t)(len - prefix)); }
            else memcpy(buf + (left ? 0 : pad), text, (size_t)len);
            wr64(ios + 0x18, 0);
            int64_t total = len + pad, done = 0;
            while (done < total) {
                uint64_t p = rd64(sb + 0x30), e = rd64(sb + 0x38);
                if (p < e) {
                    int64_t k = (int64_t)(e - p) < total - done ? (int64_t)(e - p) : total - done;
                    web_write(p, buf + done, (size_t)k);
                    wr64(sb + 0x30, p + (uint64_t)k);
                    done += k;
                } else {
                    c->x[0] = sb; c->x[1] = (uint8_t)buf[done];
                    call_indirect(c, rd64(rd64(sb) + 0x68));
                    if ((uint32_t)c->x[0] == 0xffffffffu) break;
                    done++;
                }
            }
            if (done != total) {
                uint64_t ios2 = os + rd64(rd64(os) - 0x18);
                wr32(ios2 + 0x20, rd32(ios2 + 0x20) | 1u);
            }
        } else {
            wr32(ios + 0x20, rd32(ios + 0x20) | 4u);
        }
        c->x[30] = save_lr;
        c->x[0] = os; return;
    }
    if (!strcmp(name, "_ZNSt3__113basic_istreamIcNS_11char_traitsIcEEE3getEv")) {
        uint64_t is = c->x[0], ios = is + rd64(rd64(is) - 0x18), sb = rd64(ios + 0x28), save_lr = c->x[30];
        uint32_t ch = 0xffffffffu;
        wr64(is + 8, 0);
        if (rd32(ios + 0x20) == 0 && sb) {
            uint64_t g = rd64(sb + 0x18);
            if (g < rd64(sb + 0x20)) { wr64(sb + 0x18, g + 1); ch = rd8(g); }
            else { c->x[0] = sb; call_indirect(c, rd64(rd64(sb) + 0x50)); ch = (uint32_t)c->x[0]; }
        }
        uint64_t ios2 = is + rd64(rd64(is) - 0x18);
        if (ch == 0xffffffffu) wr32(ios2 + 0x20, rd32(ios2 + 0x20) | 6u);
        else wr64(is + 8, 1);
        c->x[30] = save_lr;
        c->x[0] = ch; return;
    }
    if (!strcmp(name, "_ZNSt3__113basic_istreamIcNS_11char_traitsIcEEErsERf")) {
        uint64_t is = c->x[0], out = c->x[1], ios = is + rd64(rd64(is) - 0x18), sb = rd64(ios + 0x28), save_lr = c->x[30];
        if (rd32(ios + 0x20) != 0 || !sb) {
            wr32(ios + 0x20, rd32(ios + 0x20) | 4u);
            c->x[0] = is; return;
        }
        char text[80];
        int n = 0;
        #define IS_PEEK() ({ uint64_t _g = rd64(sb + 0x18); uint32_t _ch; if (_g < rd64(sb + 0x20)) _ch = rd8(_g); else { c->x[0] = sb; call_indirect(c, rd64(rd64(sb) + 0x48)); _ch = (uint32_t)c->x[0]; } _ch; })
        #define IS_TAKE() ({ uint64_t _g = rd64(sb + 0x18); uint32_t _ch; if (_g < rd64(sb + 0x20)) { wr64(sb + 0x18, _g + 1); _ch = rd8(_g); } else { c->x[0] = sb; call_indirect(c, rd64(rd64(sb) + 0x50)); _ch = (uint32_t)c->x[0]; } _ch; })
        uint32_t ch = IS_PEEK();
        if (rd32((is + rd64(rd64(is) - 0x18)) + 8) & 0x1000u)
            while (ch == ' ' || (ch >= 9 && ch <= 13)) { (void)IS_TAKE(); ch = IS_PEEK(); }
        while (ch != 0xffffffffu && n < (int)sizeof text - 1 &&
               (strchr("+-.eExXpP", (int)ch) || (ch >= '0' && ch <= '9') || (ch | 0x20) == 'n' || (ch | 0x20) == 'a' ||
                (ch | 0x20) == 'i' || (ch | 0x20) == 'f' || ((ch | 0x20) >= 'a' && (ch | 0x20) <= 'f' && n > 1 && (text[1] | 0x20) == 'x'))) {
            text[n++] = (char)ch;
            (void)IS_TAKE();
            ch = IS_PEEK();
        }
        #undef IS_PEEK
        #undef IS_TAKE
        text[n] = 0;
        char *end = text;
        float v = strtof(text, &end);
        ios = is + rd64(rd64(is) - 0x18);
        if (end == text) { v = 0.0f; wr32(ios + 0x20, rd32(ios + 0x20) | 4u); }
        if (ch == 0xffffffffu) wr32(ios + 0x20, rd32(ios + 0x20) | 2u);
        if (out) web_write(out, &v, 4);
        c->x[30] = save_lr;
        c->x[0] = is; return;
    }
    if (!strcmp(name, "write")) { c->x[0] = c->x[2]; return; }
    if (!strcmp(name, "setjmp")) {
        typedef struct {
            jmp_buf host;
            uint64_t x[13];
            uint64_t d[8];
            int import_symbol;
        } JmpRec;
        Ctx *volatile cc = c;
        volatile uint64_t buf = c->x[0];
        uint64_t site = c->x[30];
        if (rd32(site) != 0x35000080u || rd32(site + 4) != 0xaa1303e0u || rd32(site + 8) != 0xaa1503e1u ||
            rd32(site + 12) != 0xd63f0280u)
            guest_trap("unsupported setjmp call site", site);
        JmpRec r;
        for (int i = 0; i < 12; i++) r.x[i] = c->x[19 + i];
        r.x[12] = c->x[31];
        for (int i = 0; i < 8; i++) r.d[i] = c->v[8 + i].d[0];
        RuntimeFrame *volatile frame = active_frame();
        r.import_symbol = frame ? frame->import_symbol : -1;
        wr64(buf, 0x4a4d5052454330ULL);
        wr64(buf + 8, (uint64_t)(uintptr_t)&r);
        if (setjmp(r.host) == 0) {
            c->x[0] = c->x[19];
            c->x[1] = c->x[21];
            c->x[30] = site + 16;
            if (frame) frame->import_symbol = -1;
            call_indirect(c, c->x[20]);
        } else {
            Ctx *g = cc;
            for (int i = 0; i < 12; i++) g->x[19 + i] = r.x[i];
            g->x[31] = r.x[12];
            for (int i = 0; i < 8; i++) { g->v[8 + i].d[0] = r.d[i]; g->v[8 + i].d[1] = 0; }
        }
        if (frame) frame->import_symbol = r.import_symbol;
        wr64(buf, 0);
        cc->x[0] = 1;
        return;
    }
    if (!strcmp(name, "longjmp")) {
        if (!c->x[0] || rd64(c->x[0]) != 0x4a4d5052454330ULL)
            guest_trap("invalid longjmp buffer", c->x[0]);
        typedef struct {
            jmp_buf host;
            uint64_t x[13];
            uint64_t d[8];
            int import_symbol;
        } JmpRec;
        JmpRec *r = (JmpRec *)(uintptr_t)rd64(c->x[0] + 8);
        longjmp(r->host, 1);
    }
    if (!strcmp(name, "strdup")) {
        uint64_t src = c->x[0], len = 0;
        while (rd8(src + len)) len++;
        uint64_t save[] = {c->x[1], c->x[2], c->x[30]};
        c->x[0] = 16; c->x[1] = len + 1;
        call_indirect(c, 0x710392dce0ULL);
        uint64_t dst = c->x[0];
        c->x[1] = save[0]; c->x[2] = save[1]; c->x[30] = save[2];
        if (dst) memory_copy(dst, src, len + 1);
        c->x[0] = dst; return;
    }
    if (!strncmp(name, "glslc", 5)) {
        if (!strcmp(name, "glslcSetAllocator")) return;
        guest_trap("glslc runtime compiler called", c->x[30]);
    }
    if (!strcmp(name, "__floatditf") || !strcmp(name, "__floatsitf") || !strcmp(name, "__floatunditf") || !strcmp(name, "__floatunsitf")) {
        int is_signed = !strcmp(name, "__floatditf") || !strcmp(name, "__floatsitf");
        int is_32 = !strcmp(name, "__floatsitf") || !strcmp(name, "__floatunsitf");
        int64_t sv = is_32 ? (int64_t)(int32_t)c->x[0] : (int64_t)c->x[0];
        uint64_t sign = (is_signed && sv < 0) ? 1ULL : 0ULL;
        uint64_t u = is_signed ? (sv < 0 ? 0ULL - (uint64_t)sv : (uint64_t)sv) : (is_32 ? (uint64_t)(uint32_t)c->x[0] : c->x[0]);
        if (!u) { c->v[0].d[0] = 0; c->v[0].d[1] = 0; return; }
        int sh = __builtin_clzll(u);
        uint64_t norm = u << sh;
        c->v[0].d[0] = norm << 49;
        c->v[0].d[1] = (sign << 63) | ((uint64_t)(16383 + 63 - sh) << 48) | ((norm & 0x7fffffffffffffffULL) >> 15);
        return;
    }
    if (!strcmp(name, "__letf2") || !strcmp(name, "__lttf2") || !strcmp(name, "__eqtf2") || !strcmp(name, "__netf2") ||
        !strcmp(name, "__cmptf2") || !strcmp(name, "__getf2") || !strcmp(name, "__gttf2") || !strcmp(name, "__unordtf2")) {
        uint64_t alo = c->v[0].d[0], ahi = c->v[0].d[1], blo = c->v[1].d[0], bhi = c->v[1].d[1];
        uint64_t aabs = ahi & 0x7fffffffffffffffULL, babs = bhi & 0x7fffffffffffffffULL, inf = 0x7fff000000000000ULL;
        int nan = (aabs > inf || (aabs == inf && alo != 0)) || (babs > inf || (babs == inf && blo != 0));
        int r = 0;
        if (nan) r = 2;
        else if ((aabs == 0 && alo == 0) && (babs == 0 && blo == 0)) r = 0;
        else {
            int sa = (int)(ahi >> 63), sb = (int)(bhi >> 63);
            if (sa != sb) r = sa ? -1 : 1;
            else if (ahi == bhi && alo == blo) r = 0;
            else { int lt = (ahi < bhi) || (ahi == bhi && alo < blo); r = (lt != sa) ? -1 : 1; }
        }
        if (!strcmp(name, "__unordtf2")) c->x[0] = (r == 2);
        else if (!strcmp(name, "__getf2") || !strcmp(name, "__gttf2")) c->x[0] = (uint64_t)(int64_t)(r == 2 ? -1 : r);
        else c->x[0] = (uint64_t)(int64_t)(r == 2 ? 1 : r);
        return;
    }
    if (!strcmp(name, "__addtf3") || !strcmp(name, "__subtf3") || !strcmp(name, "__multf3") || !strcmp(name, "__divtf3")) {
        double vals[2];
        for (int k = 0; k < 2; k++) {
            uint64_t lo = c->v[k].d[0], hi = c->v[k].d[1];
            uint64_t sign = hi >> 63, exp = (hi >> 48) & 0x7fffULL, frac = ((hi & 0xffffffffffffULL) << 4) | (lo >> 60);
            uint64_t dbits = 0;
            if (exp == 0x7fffULL) dbits = (sign << 63) | (0x7ffULL << 52) | (frac | (lo != 0 || (hi & 0xffffffffffffULL) != 0 ? (1ULL << 51) : 0));
            else if (exp != 0) {
                int64_t dexp = (int64_t)exp - 16383 + 1023;
                if (dexp >= 0x7ff) dbits = (sign << 63) | (0x7ffULL << 52);
                else if (dexp > 0) dbits = (sign << 63) | ((uint64_t)dexp << 52) | frac;
            }
            memcpy(&vals[k], &dbits, 8);
        }
        double res = !strcmp(name, "__addtf3") ? vals[0] + vals[1] : !strcmp(name, "__subtf3") ? vals[0] - vals[1] : !strcmp(name, "__multf3") ? vals[0] * vals[1] : vals[0] / vals[1];
        uint64_t bits = 0; memcpy(&bits, &res, 8);
        uint64_t sign = bits >> 63, dexp = (bits >> 52) & 0x7ffULL, dfrac = bits & 0xfffffffffffffULL;
        if (dexp == 0x7ffULL) {
            c->v[0].d[0] = dfrac << 60;
            c->v[0].d[1] = (sign << 63) | (0x7fffULL << 48) | (dfrac ? (1ULL << 47) | (dfrac >> 4) : 0);
        } else if (dexp == 0 && dfrac == 0) {
            c->v[0].d[0] = 0; c->v[0].d[1] = sign << 63;
        } else {
            int64_t qexp = (int64_t)dexp - 1023 + 16383;
            if (dexp == 0) { int sh = __builtin_clzll(dfrac) - 11; dfrac = (dfrac << sh) & 0xfffffffffffffULL; qexp = 1 - sh - 1023 + 16383; }
            c->v[0].d[0] = dfrac << 60;
            c->v[0].d[1] = (sign << 63) | ((uint64_t)qexp << 48) | (dfrac >> 4);
        }
        return;
    }
    for (unsigned i = 0; i < web_sdk_leaf_count; i++) {
        if (!strcmp(name, web_sdk_leaf_imports[i].name)) { web_sdk_leaf_imports[i].fn(c); return; }
    }
    active_frame()->import_symbol = symbol;
    guest_trap("unported import", c->x[30]);
}
void host_import(Ctx *c,int symbol){
    RuntimeFrame *frame=active_frame();int previous=frame?frame->import_symbol:-1;
    if(frame)frame->import_symbol=symbol>=0&&symbol<dyn_sym_count?symbol:-1;
    host_import_body(c,symbol);
    // The heap frame belongs to this logical stack and remains stable when a
    // guest fiber resumes on another pthread. Never cache a native TLS address.
    if(frame)frame->import_symbol=previous;
}
const char *runtime_info(void) { return result; }
int runtime_run(void) {
    void *previous=web_runtime_frame_get();RuntimeFrame *frame=calloc(1,sizeof *frame);
    if(!frame)abort();frame->previous=previous;frame->import_symbol=-1;web_runtime_frame_set(frame);
    constructor_index = completed = import_calls = 0; exit_count = 0;
    constructors_ready = 0;
    memset(&context, 0, sizeof context);
    if (!elf_memory()) { snprintf(result, sizeof result, "{\"ok\":false,\"error\":\"ELF not loaded\"}");web_runtime_frame_set(previous);free(frame);return 0; }
    if (!setjmp(frame->boundary)) {
        bind_sdk_data();
        web_os_reset();
        if (!elf_relocate()) guest_trap("relocation binding failed", 0);
        if (!startup_run()) guest_trap("startup verification failed", 0);
        startup_copy_context(&context);
        context.tpidr = main_tls;
        web_memory_bind(elf_memory());
        context.x[30] = 0x7300000000ULL;
        for (; constructor_index < constructor_count; constructor_index++) {
            uint64_t function = constructor_addresses[constructor_index];
            if (!function) continue;
            context.x[30] = 0x7300000000ULL;
            uint64_t stack_before = context.x[31];
            call_indirect(&context, function);
            if (context.x[31] != stack_before) guest_trap("constructor did not restore SP", function);
            completed++;
        }
    }
    web_memory_bind(NULL);
    const char *name = frame->import_symbol >= 0 ? dyn_syms[frame->import_symbol].name : "";
    snprintf(result, sizeof result, "{\"ok\":%s,\"constructorsCompleted\":%u,\"constructorsTotal\":%u,\"constructorIndex\":%u,\"importCalls\":%u,\"exitCallbacks\":%zu,\"error\":\"%s\",\"import\":\"%s\",\"address\":\"%llx\",\"guestFunction\":\"%llx\",\"nnMainRun\":false}",
             completed == constructor_count ? "true" : "false", completed, constructor_count, constructor_index, import_calls, exit_count, frame->failure, name,
             (unsigned long long)frame->failure_address, (unsigned long long)context.fn);
    constructors_ready = completed == constructor_count;
    web_runtime_frame_set(previous);free(frame);return constructors_ready;
}
__attribute__((noinline)) static void send_runtime_report(const char *text) {
    MAIN_THREAD_ASYNC_EM_ASM({
        if (Module['onRuntimeReport']) Module['onRuntimeReport'](UTF8ToString($0));
    }, text);
}
static int protected_call(Ctx *c,void (*body)(Ctx *,void *),void *argument) {
    void *previous=web_runtime_frame_get();RuntimeFrame *frame=calloc(1,sizeof *frame);
    if(!frame)abort();frame->previous=previous;frame->import_symbol=-1;web_runtime_frame_set(frame);
    if (!setjmp(frame->boundary)) {
        body(c,argument);web_runtime_frame_set(previous);free(frame);return 1;
    }
    unsigned expected = 0;
    if (atomic_compare_exchange_strong(&thread_failed, &expected, 1)) {
        snprintf(thread_failure, sizeof thread_failure, "{\"ok\":false,\"phase\":\"guestThread\",\"constructorsCompleted\":%u,\"importCalls\":%u,\"error\":\"%s\",\"import\":\"%s\",\"address\":\"%llx\",\"guestFunction\":\"%llx\",\"lr\":\"%llx\",\"sp\":\"%llx\",\"x0\":\"%llx\",\"x8\":\"%llx\",\"x19\":\"%llx\",\"x20\":\"%llx\",\"x21\":\"%llx\",\"x24\":\"%llx\",\"x28\":\"%llx\",\"threadFailed\":1,\"nnMainRun\":true}",
            completed, import_calls, frame->failure, frame->import_symbol >= 0 ? dyn_syms[frame->import_symbol].name : "", (unsigned long long)frame->failure_address, (unsigned long long)c->fn,
            (unsigned long long)c->x[30], (unsigned long long)c->x[31], (unsigned long long)c->x[0], (unsigned long long)c->x[8], (unsigned long long)c->x[19], (unsigned long long)c->x[20], (unsigned long long)c->x[21], (unsigned long long)c->x[24], (unsigned long long)c->x[28]);
        send_runtime_report(thread_failure);
    }
    web_runtime_frame_set(previous);free(frame);return 0;
}
static void thread_body(Ctx *context,void *argument){
    uint64_t function=*(uint64_t *)argument;
    if(!function)guest_trap("cannot map thread TLS",0);
    call_indirect(context,function);
}
int web_runtime_thread_call(Ctx *context,uint64_t function){return protected_call(context,thread_body,&function);}
int web_runtime_fiber_call(Ctx *context,void (*body)(Ctx *,void *),void *argument){return protected_call(context,body,argument);}
static void *runtime_driver(void *argument) {
    int run_main = (int)(uintptr_t)argument;
    if (runtime_run() && run_main) runtime_main();
    if (!atomic_load(&thread_failed)) send_runtime_report(result);
#ifdef WEB_FIBERS
    web_fiber_thread_cleanup();
#endif
    pthread_exit(NULL);
}
int runtime_start(int run_main) {
    atomic_store(&thread_failed, 0);
    pthread_attr_t attributes;
    pthread_attr_init(&attributes); pthread_attr_setstacksize(&attributes, 4194304);
    int rc = pthread_create(&driver_thread, &attributes, runtime_driver, (void *)(uintptr_t)run_main);
    driver_started = rc == 0;
    pthread_attr_destroy(&attributes); return rc;
}
int runtime_join(void) { if (!driver_started) return 0; int result = pthread_join(driver_thread, NULL); if (!result) driver_started = 0; return result; }
int runtime_main(void) {
    if (!constructors_ready) {
        snprintf(result, sizeof result, "{\"ok\":false,\"error\":\"Constructors not complete\",\"nnMainRun\":false}"); return 0;
    }
    void *previous=web_runtime_frame_get();RuntimeFrame *frame=calloc(1,sizeof *frame);
    if(!frame)abort();frame->previous=previous;frame->import_symbol=-1;web_runtime_frame_set(frame);
    int returned = 0;
    if (!setjmp(frame->boundary)) {
        web_memory_bind(elf_memory());
        context.x[30] = 0x7300000000ULL;
        call_indirect(&context, 0x71002c5620ULL);
        returned = 1;
    }
    web_memory_bind(NULL);
    snprintf(result, sizeof result, "{\"ok\":%s,\"phase\":\"nnMain\",\"constructorsCompleted\":%u,\"importCalls\":%u,\"error\":\"%s\",\"import\":\"%s\",\"address\":\"%llx\",\"guestFunction\":\"%llx\",\"lr\":\"%llx\",\"sp\":\"%llx\",\"x0\":\"%llx\",\"x8\":\"%llx\",\"x19\":\"%llx\",\"x20\":\"%llx\",\"x21\":\"%llx\",\"x24\":\"%llx\",\"x28\":\"%llx\",\"threadFailed\":%u,\"nnMainRun\":true,\"nnMainReturned\":%s}",
             returned ? "true" : "false", completed, import_calls, frame->failure, frame->import_symbol >= 0 ? dyn_syms[frame->import_symbol].name : "",
             (unsigned long long)frame->failure_address, (unsigned long long)context.fn,
             (unsigned long long)context.x[30], (unsigned long long)context.x[31], (unsigned long long)context.x[0], (unsigned long long)context.x[8], (unsigned long long)context.x[19], (unsigned long long)context.x[20], (unsigned long long)context.x[21], (unsigned long long)context.x[24], (unsigned long long)context.x[28],
             atomic_load(&thread_failed), returned ? "true" : "false");
    web_runtime_frame_set(previous);free(frame);return returned;
}
