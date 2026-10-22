/* Self-authored synthetic programs. No instructions or data from a game dump. */
#include "guest_memory.h"
#include "runtime.h" /* exercise the real portable arithmetic/SIMD helpers */
#include <emscripten.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int checks, failures;
static const GuestAddr BASE = UINT64_C(0x7100000000);
static void check(const char *name, int ok) {
    ++checks; if (!ok) ++failures;
    printf("SSBU_CHECK {\"name\":\"%s\",\"ok\":%s}\n", name, ok ? "true" : "false");
}
static void memory_tests(void) {
    GuestMemory m;
    gm_init(&m);
    check("reserve-16GiB-with-no-backing", gm_reserve(&m, BASE, UINT64_C(16) << 30) == GM_OK && m.backing_pages == 0);
    check("reject-overlapping-reservation", gm_reserve(&m, BASE, GM_PAGE_SIZE) == GM_OVERLAP);
    check("reject-null-and-overflow", gm_reserve(&m, 0, GM_PAGE_SIZE) == GM_RANGE && gm_reserve(&m, UINT64_MAX - 4095, 8192) == GM_RANGE);
    uint64_t word = 0xdeadbeef, out = 0xabcdef;
    check("uncommitted-read-preserves-output", gm_read(&m, BASE, &out, 8) == GM_UNMAPPED && out == 0xabcdef);
    check("commit-only-three-sparse-pages", gm_commit(&m, BASE, GM_PAGE_SIZE * 2) == GM_OK && gm_commit(&m, BASE + (UINT64_C(16) << 30) - GM_PAGE_SIZE, GM_PAGE_SIZE) == GM_OK && m.backing_pages == 3);
    out = 99;
    check("new-page-is-zero", gm_read(&m, BASE, &out, 8) == GM_OK && out == 0);
    uint8_t a8 = 0xa5, b8 = 0;
    uint16_t a16 = 0xbcde, b16 = 0;
    uint32_t a32 = 0x12345678, b32 = 0;
    check("scalar-8-16-32bit-unaligned-roundtrips",
          gm_write(&m, BASE + 101, &a8, 1) == GM_OK && gm_read(&m, BASE + 101, &b8, 1) == GM_OK && b8 == a8 &&
          gm_write(&m, BASE + 103, &a16, 2) == GM_OK && gm_read(&m, BASE + 103, &b16, 2) == GM_OK && b16 == a16 &&
          gm_write(&m, BASE + 107, &a32, 4) == GM_OK && gm_read(&m, BASE + 107, &b32, 4) == GM_OK && b32 == a32);
    check("commit-capacity-does-not-partially-map", gm_commit(&m, BASE + GM_PAGE_SIZE * 4, GM_PAGE_SIZE * (GM_MAX_PAGES + 1)) == GM_CAPACITY && m.backing_pages == 3);
    unsigned char pattern[16], got[16];
    for (unsigned i = 0; i < sizeof pattern; ++i) pattern[i] = (unsigned char)(i * 13 + 5);
    check("128bit-unaligned-cross-page-roundtrip", gm_write(&m, BASE + GM_PAGE_SIZE - 7, pattern, sizeof pattern) == GM_OK && gm_read(&m, BASE + GM_PAGE_SIZE - 7, got, sizeof got) == GM_OK && !memcmp(pattern, got, sizeof got));
    check("cross-page-marks-both-pages", gm_dirty(&m, BASE) && gm_dirty(&m, BASE + GM_PAGE_SIZE));
    gm_clear_dirty(&m);
    check("quiescent-dirty-reset", !gm_dirty(&m, BASE) && !gm_dirty(&m, BASE + GM_PAGE_SIZE));
    void *span = (void *)1;
    check("cross-page-span-rejected", gm_span(&m, BASE + GM_PAGE_SIZE - 7, 16, 0, &span) == GM_RANGE && !span);
    const GuestAddr ALIAS = UINT64_C(0x7200000000);
    /* ALIAS is inside the existing 16 GiB reservation, four GiB after BASE. */
    check("alias-shares-backing", gm_alias(&m, ALIAS, BASE, GM_PAGE_SIZE) == GM_OK && m.backing_pages == 3);
    check("alias-write-visible-at-original", gm_write(&m, ALIAS + 24, &word, 8) == GM_OK && gm_read(&m, BASE + 24, &out, 8) == GM_OK && out == word && gm_dirty(&m, BASE));
    check("page-permissions", gm_protect(&m, BASE + GM_PAGE_SIZE, GM_PAGE_SIZE, 0) == GM_OK);
    gm_read(&m, BASE + GM_PAGE_SIZE - 7, got, sizeof got);
    unsigned char before[16]; memcpy(before, got, 16);
    memset(pattern, 0x77, sizeof pattern);
    int readonly = gm_write(&m, BASE + GM_PAGE_SIZE - 7, pattern, 16) == GM_READONLY;
    gm_read(&m, BASE + GM_PAGE_SIZE - 7, got, 16);
    check("failed-write-is-not-partial", readonly && !memcmp(before, got, 16));
    gm_write(&m, BASE + 64, &word, 8);
    uint64_t expected = word; int exchanged = 0;
    check("64bit-cas-success", gm_cas64(&m, BASE + 64, &expected, 17, &exchanged) == GM_OK && exchanged);
    expected = 9; exchanged = 1;
    check("64bit-cas-failure-updates-expected", gm_cas64(&m, BASE + 64, &expected, 18, &exchanged) == GM_OK && !exchanged && expected == 17);
    check("unaligned-cas-rejected", gm_cas64(&m, BASE + 65, &expected, 18, &exchanged) == GM_ALIGNMENT);
    check("decommit-original-preserves-alias", gm_decommit(&m, BASE, GM_PAGE_SIZE) == GM_OK && m.backing_pages == 3 && gm_read(&m, ALIAS + 24, &out, 8) == GM_OK && out == word);
    check("decommitted-range-inaccessible", gm_read(&m, BASE, &out, 8) == GM_UNMAPPED);
    check("recommit-zeroes-and-breaks-alias", gm_commit(&m, BASE, GM_PAGE_SIZE) == GM_OK && gm_read(&m, BASE + 24, &out, 8) == GM_OK && out == 0 && m.backing_pages == 4);
    check("read-overflow-rejected", gm_read(&m, UINT64_MAX - 3, got, 16) == GM_RANGE);
    check("zero-length-copy", gm_read(&m, 0, NULL, 0) == GM_OK && gm_write(&m, 0, NULL, 0) == GM_OK);
    GmStatus gm_os_commit(GuestMemory *, GuestAddr, size_t);
    GmStatus gm_os_decommit(GuestMemory *, GuestAddr, size_t);
    const GuestAddr OS_BASE = BASE + 0x100000;
    uint64_t os_val = 0xfeedfacecafebeefULL, os_out = 0;
    check("os-commit-and-idempotent-recommit",
          gm_os_commit(&m, OS_BASE + GM_PAGE_SIZE, GM_PAGE_SIZE) == GM_OK &&
          gm_write(&m, OS_BASE + GM_PAGE_SIZE, &os_val, 8) == GM_OK &&
          gm_os_commit(&m, OS_BASE, GM_PAGE_SIZE * 3) == GM_OK &&
          gm_read(&m, OS_BASE + GM_PAGE_SIZE, &os_out, 8) == GM_OK && os_out == os_val &&
          gm_read(&m, OS_BASE, &os_out, 8) == GM_OK && os_out == 0);
    check("os-decommit-partial-and-uncommitted",
          gm_os_decommit(&m, OS_BASE + GM_PAGE_SIZE, GM_PAGE_SIZE) == GM_OK &&
          gm_os_decommit(&m, OS_BASE, GM_PAGE_SIZE * 4) == GM_OK &&
          gm_read(&m, OS_BASE, &os_out, 8) == GM_UNMAPPED);
    gm_destroy(&m);
    check("destroy-releases-alias-and-backing", m.backing_pages == 0 && m.page_count == 0 && m.region_count == 0);
    GuestMemory big;
    gm_init_limit(&big, GM_HARD_MAX_PAGES);
    check("lazy-commit-1GiB-range",
          gm_reserve(&big, BASE, UINT64_C(2) << 30) == GM_OK &&
          gm_os_commit(&big, BASE, 1u << 30) == GM_OK &&
          big.page_count == 262144 && big.backing_pages == 262144 &&
          gm_read(&big, BASE + (512u << 20), &os_out, 8) == GM_OK && os_out == 0 &&
          gm_os_decommit(&big, BASE, 1u << 30) == GM_OK &&
          big.page_count == 0 && big.backing_pages == 0);
    gm_destroy(&big);
}

/* A lifted-style synthetic function using Ctx, explicit guest SP and typed calls. */
typedef void (*SyntheticFn)(Ctx *, GuestMemory *);
static void synthetic_add(Ctx *c, GuestMemory *m) {
    (void)m;
    uint64_t previous = rd64(c->x[31]);
    c->x[0] = adds64(c, previous, c->x[1], 0);
    wr64(c->x[31], c->x[0]);
}
static int exclusive_misses;
void stx_diag(Ctx *c, uint64_t a, uint64_t expected, uint64_t seen, uint64_t v, int bits) {
    (void)c; (void)a; (void)expected; (void)seen; (void)v; (void)bits;
    ++exclusive_misses;
}
static void cpu_tests(void) {
    Ctx c = {0};
    check("upstream-add64-carry", adds64(&c, UINT64_MAX, 1, 0) == 0 && c.C && c.Z && !c.V);
    check("upstream-add32-overflow", adds32(&c, 0x7fffffff, 1, 0) == 0x80000000 && c.V && c.N && !c.C);
    fcmp(&c, NAN, 1);
    check("upstream-fcmp-unordered", c.C && c.V && !c.N && !c.Z);
    check("upstream-int-division-edges", sdiv64(1, 0) == 0 && sdiv64(UINT64_C(0x8000000000000000), UINT64_MAX) == UINT64_C(0x8000000000000000));
    check("upstream-multiply-high", umulh(UINT64_MAX, UINT64_MAX) == UINT64_MAX - 1 && smulh(UINT64_MAX, 2) == UINT64_MAX);
    check("upstream-saturating-conversion", f2u32(-1) == 0 && f2s32(INFINITY) == 0x7fffffff && f2u64(NAN) == 0);
    check("upstream-estimate-helpers", recpe_f(1.0f) == 0.998046875f && isinf(recpe_f(0.0f)) && isnan(rsqrte_f(-1.0f)));
    V128 v = {0}; vset(&v, 2, 3, 0x12345678);
    check("upstream-vector-lanes", vget(&v, 2, 3) == 0x12345678 && vget(&v, 2, 2) == 0);
    GuestMemory m; gm_init(&m);
    int ok = gm_reserve(&m, BASE, 8192) == GM_OK && gm_commit(&m, BASE, 8192) == GM_OK;
    if (!ok) { check("runtime-adapter-setup", 0); return; }
    web_memory_bind(&m);
    c = (Ctx){0}; c.x[31] = BASE + 4093; /* frame word crosses a page */
    SyntheticFn fn = synthetic_add;
    for (unsigned i = 1; i <= 1000 && ok; ++i) { c.x[1] = i; fn(&c, &m); }
    uint64_t sum = 0;
    ok = ok && gm_read(&m, c.x[31], &sum, 8) == GM_OK && sum == 500500;
    check("synthetic-ctx-indirect-call-high-stack", ok && c.x[0] == 500500);
    wr8(BASE + 11, 0x7a); wr16(BASE + 13, 0x9876); wr32(BASE + 17, 0x12345678);
    check("runtime-scalar-helpers-use-mapping", rd8(BASE + 11) == 0x7a && rd16(BASE + 13) == 0x9876 && rd32(BASE + 17) == 0x12345678);
    V128 vector = {0}; vector.d[0] = 0x12345678; vector.d[1] = UINT64_MAX;
    wrv(BASE + 4085, &vector, 16);
    V128 copied = rdv(BASE + 4085, 16), half = rdv(BASE + 4085, 8);
    check("runtime-vector-helpers-cross-pages", !memcmp(&vector, &copied, 16) && half.d[0] == vector.d[0] && half.d[1] == 0);
    wr64(BASE + 64, 7);
    c.ex_addr = BASE + 64; c.ex_val = 7;
    check("runtime-stx64-success-and-monitor-reset", stx64(&c, BASE + 64, 9) == 0 && c.ex_addr == UINT64_MAX && rd64(BASE + 64) == 9);
    c.ex_addr = BASE + 64; c.ex_val = 7;
    int misses = exclusive_misses;
    check("runtime-stx64-conflict-and-address-mismatch", stx64(&c, BASE + 64, 10) == 1 && stx64(&c, BASE + 64, 10) == 1 && exclusive_misses == misses + 2 && rd64(BASE + 64) == 9);
    wr8(BASE + 80, 1); c.ex_addr = BASE + 80; c.ex_val = 1;
    int small = stx8(&c, BASE + 80, 2) == 0 && rd8(BASE + 80) == 2;
    wr16(BASE + 82, 3); c.ex_addr = BASE + 82; c.ex_val = 3;
    small = small && stx16(&c, BASE + 82, 4) == 0 && rd16(BASE + 82) == 4;
    wr32(BASE + 84, 5); c.ex_addr = BASE + 84; c.ex_val = 5;
    check("runtime-stx8-16-32-widths", small && stx32(&c, BASE + 84, 6) == 0 && rd32(BASE + 84) == 6);
    web_memory_bind(NULL);
    gm_destroy(&m);
}

static GuestMemory shared_memory;
static atomic_int thread_failed;
static _Thread_local unsigned local_marker;
static uint64_t atomic_increment(GuestAddr address) {
    uint64_t expected = 0;
    for (;;) {
        uint64_t prior = expected;
        int changed = 0;
        if (gm_cas64(&shared_memory, address, &expected, prior + 1, &changed) != GM_OK) { atomic_store(&thread_failed, 1); return 0; }
        if (changed) return prior + 1;
    }
}
static void *counter_worker(void *arg) {
    unsigned id = (unsigned)(uintptr_t)arg;
    local_marker = id;
    if (web_memory_current()) atomic_store(&thread_failed, 1);
    web_memory_bind(&shared_memory);
    for (unsigned i = 0; i < 10000; ++i) atomic_increment(BASE);
    if (local_marker != id || web_memory_current() != &shared_memory) atomic_store(&thread_failed, 1);
    web_memory_bind(NULL);
    return NULL;
}
static atomic_uint mutation_started, mutation_finished, mutation_reads;
static const GuestAddr MUTABLE = UINT64_C(0x7100010000);
static void *mapping_writer(void *unused) {
    (void)unused;
    while (!atomic_load(&mutation_started)) { }
    uint64_t value = UINT64_C(0xaabbccddeeff1234);
    for (unsigned i = 0; i < 250; ++i) {
        if (gm_commit(&shared_memory, MUTABLE, 160 * GM_PAGE_SIZE) != GM_OK ||
            gm_write(&shared_memory, MUTABLE + 4093, &value, 8) != GM_OK ||
            gm_protect(&shared_memory, MUTABLE, 160 * GM_PAGE_SIZE, 0) != GM_OK ||
            gm_decommit(&shared_memory, MUTABLE, 160 * GM_PAGE_SIZE) != GM_OK)
            atomic_store(&thread_failed, 1);
    }
    atomic_store(&mutation_finished, 1);
    return NULL;
}
static void *mapping_reader(void *unused) {
    (void)unused;
    unsigned reads = 0;
    do {
        uint64_t value = 17, stable = 0;
        GmStatus status = gm_read(&shared_memory, MUTABLE + 4093, &value, 8);
        if (!((status == GM_UNMAPPED && value == 17) ||
              (status == GM_OK && (value == 0 || value == UINT64_C(0xaabbccddeeff1234)))) ||
            gm_read(&shared_memory, BASE, &stable, 8) != GM_OK || stable != 20000)
            atomic_store(&thread_failed, 1);
        ++reads;
        atomic_store(&mutation_started, 1);
    } while (!atomic_load(&mutation_finished) || reads < 1000);
    atomic_store(&mutation_reads, reads);
    return NULL;
}
/* An explicit continuation can migrate: all persistent task state is in Task.
 * This is NOT a port of the game's stackful fibers. */
typedef struct { unsigned pc, yields, first_worker, last_worker; uint64_t sum; int ok; } Task;
typedef struct { Task *task; unsigned id; } ResumeArg;
static pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t task_changed = PTHREAD_COND_INITIALIZER;
static int task_phase;
static void *resume_task(void *arg) {
    ResumeArg *resume = arg;
    Task *task = resume->task;
    local_marker = resume->id;
    pthread_mutex_lock(&task_mutex);
    if (resume->id == 1) {
        task->first_worker = local_marker;
        task->sum = 41; task->yields++; task->pc = 1;
        task_phase = 1;
        pthread_cond_broadcast(&task_changed);
        while (task_phase != 2) pthread_cond_wait(&task_changed, &task_mutex);
        if (local_marker != 1) task->ok = 0;
    } else {
        while (task_phase != 1) pthread_cond_wait(&task_changed, &task_mutex);
        task->last_worker = local_marker;
        /* Exercise same-worker nonlocal exit after resuming elsewhere. */
        jmp_buf cancel;
        if (!setjmp(cancel)) { task->sum++; longjmp(cancel, 1); }
        else { task->ok = local_marker == 2 && task->sum == 42; task->pc = 2; }
        task_phase = 2;
        pthread_cond_broadcast(&task_changed);
    }
    pthread_mutex_unlock(&task_mutex);
    return NULL;
}
static void thread_tests(void) {
    gm_init(&shared_memory);
    int ready = gm_reserve(&shared_memory, BASE, 4096) == GM_OK && gm_commit(&shared_memory, BASE, 4096) == GM_OK;
    if (!ready) { check("thread-memory-setup", 0); return; }
    pthread_t a, b;
    int ra = pthread_create(&a, NULL, counter_worker, (void *)(uintptr_t)11);
    int rb = pthread_create(&b, NULL, counter_worker, (void *)(uintptr_t)22);
    if (!ra) pthread_join(a, NULL);
    if (!rb) pthread_join(b, NULL);
    uint64_t count = 0; gm_read(&shared_memory, BASE, &count, 8);
    check("pthreads-shared-64bit-cas-20000", !ra && !rb && count == 20000 && !atomic_load(&thread_failed));
    check("pthreads-tls-and-memory-binding-main-isolated", local_marker == 0 && !web_memory_current());
    int reserved = gm_reserve(&shared_memory, MUTABLE, 160 * GM_PAGE_SIZE) == GM_OK;
    ra = pthread_create(&a, NULL, mapping_writer, NULL);
    rb = pthread_create(&b, NULL, mapping_reader, NULL);
    if (ra || rb) abort();
    pthread_join(a, NULL); pthread_join(b, NULL);
    check("concurrent-map-grow-protect-decommit-cross-page-read", reserved &&
          atomic_load(&mutation_reads) >= 1000 && !atomic_load(&thread_failed) && shared_memory.page_count == 1);
    Task task = {0};
    /* Both pthreads remain alive: this also proves two distinct host workers. */
    ResumeArg first = {&task, 1}, second = {&task, 2};
    ra = pthread_create(&a, NULL, resume_task, &first);
    rb = pthread_create(&b, NULL, resume_task, &second);
    if (ra || rb) {
        /* A failure to start either participant cannot satisfy its peer's wait. */
        check("explicit-continuation-worker-creation", 0);
        abort();
    }
    if (!ra) pthread_join(a, NULL);
    if (!rb) pthread_join(b, NULL);
    check("explicit-continuation-across-two-live-workers", task.pc == 2 && task.yields == 1 && task.ok && task.first_worker == 1 && task.last_worker == 2);
    gm_destroy(&shared_memory);
}
static void benchmark(void) {
    GuestMemory m; gm_init(&m);
    if (gm_reserve(&m, BASE, 4096) != GM_OK || gm_commit(&m, BASE, 4096) != GM_OK) { ++failures; return; }
    double start = emscripten_get_now();
    uint64_t checksum = 0;
    for (uint64_t i = 0; i < 200000; ++i) {
        uint64_t got = 0;
        gm_write(&m, BASE + (i % 128) * 8, &i, 8);
        gm_read(&m, BASE + (i % 128) * 8, &got, 8);
        checksum += got;
    }
    double elapsed = emscripten_get_now() - start;
    check("memory-loop-checksum", checksum == UINT64_C(19999900000));
    printf("SSBU_BENCH {\"iterations\":200000,\"milliseconds\":%.3f,\"checksum\":\"%llu\",\"scope\":\"synthetic memory loop; not a game benchmark\"}\n", elapsed, (unsigned long long)checksum);
    gm_destroy(&m);
}
static void scalable_memory_tests(void) {
    GuestMemory m;
    check("dynamic-page-limit-validation", gm_init_limit(&m, 0) == GM_RANGE);
    check("dynamic-page-table-init", gm_init_limit(&m, 2048) == GM_OK && m.capacity == 0);
    check("dynamic-large-commit", gm_reserve(&m, BASE, 16 * 1024 * 1024) == GM_OK && gm_commit(&m, BASE, 1024 * GM_PAGE_SIZE) == GM_OK && m.page_count == 1024);
    const GuestAddr alias = BASE + 8 * 1024 * 1024;
    check("dynamic-growth-alias", gm_alias(&m, alias, BASE, 1024 * GM_PAGE_SIZE) == GM_OK && m.page_count == 2048 && m.backing_pages == 1024);
    uint64_t value = 1234567, got = 0;
    check("dynamic-growth-preserves-last-page", gm_write(&m, BASE + 1024 * GM_PAGE_SIZE - 8, &value, 8) == GM_OK && gm_read(&m, alias + 1024 * GM_PAGE_SIZE - 8, &got, 8) == GM_OK && got == value);
    check("dynamic-limit-no-partial-commit", gm_commit(&m, BASE + 4 * 1024 * 1024, GM_PAGE_SIZE) == GM_CAPACITY && m.page_count == 2048);
    check("bulk-decommit-preserves-alias-backing", gm_decommit(&m, BASE, 1024 * GM_PAGE_SIZE) == GM_OK && m.page_count == 1024 && m.backing_pages == 1024 && gm_read(&m, alias + 1024 * GM_PAGE_SIZE - 8, &got, 8) == GM_OK && got == value);
    check("bulk-decommit-middle-range", gm_decommit(&m, alias + 128 * GM_PAGE_SIZE, 256 * GM_PAGE_SIZE) == GM_OK && m.page_count == 768 && m.backing_pages == 768);
    check("bulk-decommit-keeps-both-neighbours", gm_read(&m, alias + 127 * GM_PAGE_SIZE, &got, 8) == GM_OK && gm_read(&m, alias + 384 * GM_PAGE_SIZE, &got, 8) == GM_OK && gm_read(&m, alias + 128 * GM_PAGE_SIZE, &got, 8) == GM_UNMAPPED);
    gm_destroy(&m);
    check("dynamic-destroy-clears-page-table", !m.pages && !m.capacity && !m.backing_pages);
}
int main(void) {
    memory_tests(); scalable_memory_tests(); cpu_tests(); thread_tests(); benchmark();
    printf("SSBU_PROBE_RESULT {\"checks\":%d,\"failures\":%d,\"pointerBits\":%zu,\"scope\":\"synthetic CPU/memory/pthreads; no game run\"}\n", checks, failures, sizeof(void *) * 8);
    return failures ? 1 : 0;
}
