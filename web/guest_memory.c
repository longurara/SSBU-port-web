#include "guest_memory.h"
#include <stdlib.h>
#include <string.h>

/* Public operations serialize table mutation with scalar/vector access. Borrowed
 * gm_span pointers still require the documented external lifetime discipline. */
#define gm_destroy gm_destroy_unlocked
#define gm_reserve gm_reserve_unlocked
#define gm_commit gm_commit_unlocked
#define gm_decommit gm_decommit_unlocked
#define gm_os_commit gm_os_commit_unlocked
#define gm_os_decommit gm_os_decommit_unlocked
#define gm_alias gm_alias_unlocked
#define gm_protect gm_protect_unlocked
#define gm_read gm_read_unlocked
#define gm_write gm_write_unlocked
#define gm_span gm_span_unlocked
#define gm_cas64 gm_cas64_unlocked
#define gm_cas gm_cas_unlocked
#define gm_dirty gm_dirty_unlocked
#define gm_clear_dirty gm_clear_dirty_unlocked
GmStatus gm_cas_unlocked(GuestMemory *, GuestAddr, unsigned, uint64_t *, uint64_t, int *);
GmStatus gm_os_commit(GuestMemory *, GuestAddr, size_t);
GmStatus gm_os_decommit(GuestMemory *, GuestAddr, size_t);

struct GmPage {
    _Alignas(16) unsigned char data[GM_PAGE_SIZE];
    atomic_uint dirty;
    unsigned refs;
};

static int valid_range(GuestAddr base, uint64_t size) {
    return base != 0 && size != 0 && size <= UINT64_MAX - base;
}
static int aligned_range(GuestAddr base, uint64_t size) {
    return valid_range(base, size) && !(base % GM_PAGE_SIZE) && !(size % GM_PAGE_SIZE);
}
static size_t lower_bound(const GuestMemory *m, GuestAddr base) {
    size_t lo = 0, hi = m->page_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (m->pages[mid].base < base) lo = mid + 1; else hi = mid;
    }
    return lo;
}
#define GM_TLB_BITS 17
#define GM_TLB_SIZE (1u << GM_TLB_BITS)
static size_t g_tlb_idx[GM_TLB_SIZE];

static GmEntry *find(const GuestMemory *m, GuestAddr a) {
    GuestAddr base = a & ~(GuestAddr)(GM_PAGE_SIZE - 1);
    size_t slot = (size_t)((base / GM_PAGE_SIZE) & (GM_TLB_SIZE - 1));
    size_t cached = g_tlb_idx[slot];
    if (cached < m->page_count && m->pages[cached].base == base)
        return (GmEntry *)&m->pages[cached];
    size_t lo = lower_bound(m, base);
    if (lo < m->page_count && m->pages[lo].base == base) {
        g_tlb_idx[slot] = lo;
        return (GmEntry *)&m->pages[lo];
    }
    return NULL;
}
static int reserved(const GuestMemory *m, GuestAddr base, uint64_t size) {
    for (size_t i = 0; i < m->region_count; ++i) {
        GmRegion r = m->regions[i];
        if (base >= r.base && base - r.base <= r.size && size <= r.size - (base - r.base)) return 1;
    }
    return 0;
}
static void insert(GuestMemory *m, GuestAddr base, GmPage *p) {
    size_t idx = lower_bound(m, base);
    memmove(&m->pages[idx + 1], &m->pages[idx], (m->page_count - idx) * sizeof *m->pages);
    m->pages[idx] = (GmEntry){base, p, 1};
    ++m->page_count;
}
static GmStatus materialize(GmEntry *e) {
    if (e->page) return GM_OK;
    GmPage *p = calloc(1, sizeof(GmPage));
    if (!p) return GM_NOMEM;
    p->refs = 1;
    atomic_init(&p->dirty, 0);
    e->page = p;
    return GM_OK;
}
static void release(GuestMemory *m, GmPage *p) {
    if (!p) { --m->backing_pages; return; }
    if (!--p->refs) { free(p); --m->backing_pages; }
}
void gm_init(GuestMemory *m) { memset(m, 0, sizeof *m); atomic_init(&m->map_lock, 0); m->page_limit = GM_MAX_PAGES; }
GmStatus gm_init_limit(GuestMemory *m, size_t page_limit) {
    gm_init(m);
    if (!page_limit || page_limit > GM_HARD_MAX_PAGES) return GM_RANGE;
    m->page_limit = page_limit;
    return GM_OK;
}
static GmStatus grow(GuestMemory *m, size_t count) {
    if (count > m->page_limit - m->page_count) return GM_CAPACITY;
    size_t needed = m->page_count + count;
    if (needed <= m->capacity) return GM_OK;
    size_t capacity = m->capacity ? m->capacity : 16;
    while (capacity < needed) capacity *= 2;
    if (capacity > m->page_limit) capacity = m->page_limit;
    GmEntry *entries = realloc(m->pages, capacity * sizeof *entries);
    if (!entries) return GM_NOMEM;
    m->pages = entries; m->capacity = capacity;
    return GM_OK;
}
void gm_destroy(GuestMemory *m) {
    for (size_t i = 0; i < m->page_count; ++i) release(m, m->pages[i].page);
    free(m->pages);
    gm_init(m);
}
GmStatus gm_reserve(GuestMemory *m, GuestAddr base, uint64_t size) {
    if (!aligned_range(base, size)) return GM_RANGE;
    for (size_t i = 0; i < m->region_count; ++i) {
        GmRegion r = m->regions[i];
        if (base < r.base + r.size && r.base < base + size) return GM_OVERLAP;
    }
    if (m->region_count == GM_MAX_REGIONS) return GM_CAPACITY;
    m->regions[m->region_count++] = (GmRegion){base, size};
    return GM_OK;
}
GmStatus gm_commit(GuestMemory *m, GuestAddr base, size_t size) {
    if (!aligned_range(base, size) || !reserved(m, base, size)) return GM_RANGE;
    size_t count = size / GM_PAGE_SIZE;
    if (count > m->page_limit - m->page_count) return GM_CAPACITY;
    size_t idx = lower_bound(m, base);
    if (idx < m->page_count && m->pages[idx].base < base + size) return GM_OVERLAP;
    GmStatus status = grow(m, count);
    if (status != GM_OK) return status;
    memmove(&m->pages[idx + count], &m->pages[idx], (m->page_count - idx) * sizeof *m->pages);
    for (size_t i = 0; i < count; ++i)
        m->pages[idx + i] = (GmEntry){base + i * GM_PAGE_SIZE, NULL, 1};
    m->page_count += count;
    m->backing_pages += count;
    return GM_OK;
}
GmStatus gm_os_commit(GuestMemory *m, GuestAddr base, size_t size) {
    if (!aligned_range(base, size) || !reserved(m, base, size)) return GM_RANGE;
    size_t count = size / GM_PAGE_SIZE;
    GuestAddr end = base + size;
    size_t idx = lower_bound(m, base), end_idx = lower_bound(m, end);
    size_t existing = end_idx - idx, missing = count - existing;
    if (missing) {
        GmStatus status = grow(m, missing);
        if (status != GM_OK) return status;
        memmove(&m->pages[end_idx + missing], &m->pages[end_idx], (m->page_count - end_idx) * sizeof *m->pages);
        size_t src = end_idx;
        for (size_t i = count; i > 0; --i) {
            GuestAddr addr = base + (i - 1) * GM_PAGE_SIZE;
            if (src > idx && m->pages[src - 1].base == addr) {
                m->pages[idx + i - 1] = m->pages[--src];
                m->pages[idx + i - 1].writable = 1;
            } else {
                m->pages[idx + i - 1] = (GmEntry){addr, NULL, 1};
            }
        }
        m->page_count += missing;
        m->backing_pages += missing;
    } else {
        for (size_t i = idx; i < end_idx; ++i) m->pages[i].writable = 1;
    }
    return GM_OK;
}
static GmStatus check(const GuestMemory *m, GuestAddr a, size_t n, int write) {
    if (!n) return GM_OK;
    if (!valid_range(a, n)) return GM_RANGE;
    while (n) {
        GmEntry *e = find(m, a);
        if (!e) return GM_UNMAPPED;
        if (write && !e->writable) return GM_READONLY;
        size_t step = GM_PAGE_SIZE - (size_t)(a % GM_PAGE_SIZE);
        if (step > n) step = n;
        a += step; n -= step;
    }
    return GM_OK;
}
GmStatus gm_decommit(GuestMemory *m, GuestAddr base, size_t size) {
    if (!aligned_range(base, size)) return GM_RANGE;
    size_t count = size / GM_PAGE_SIZE, idx = lower_bound(m, base);
    if (idx + count > m->page_count || m->pages[idx].base != base ||
        m->pages[idx + count - 1].base != base + (count - 1) * GM_PAGE_SIZE)
        return GM_UNMAPPED;
    for (size_t i = 0; i < count; i++) release(m, m->pages[idx + i].page);
    memmove(&m->pages[idx], &m->pages[idx + count], (m->page_count - idx - count) * sizeof *m->pages);
    m->page_count -= count;
    return GM_OK;
}
GmStatus gm_os_decommit(GuestMemory *m, GuestAddr base, size_t size) {
    if (!aligned_range(base, size) || !reserved(m, base, size)) return GM_RANGE;
    size_t idx = lower_bound(m, base), end_idx = lower_bound(m, base + size);
    size_t removed = end_idx - idx;
    if (!removed) return GM_OK;
    for (size_t i = idx; i < end_idx; ++i) release(m, m->pages[i].page);
    memmove(&m->pages[idx], &m->pages[end_idx], (m->page_count - end_idx) * sizeof *m->pages);
    m->page_count -= removed;
    return GM_OK;
}
GmStatus gm_alias(GuestMemory *m, GuestAddr dst, GuestAddr src, size_t size) {
    if (!aligned_range(dst, size) || !aligned_range(src, size) || !reserved(m, dst, size)) return GM_RANGE;
    GmStatus s = check(m, src, size, 0);
    if (s != GM_OK) return s;
    size_t count = size / GM_PAGE_SIZE;
    if (count > m->page_limit - m->page_count) return GM_CAPACITY;
    for (size_t i = 0; i < count; ++i) if (find(m, dst + i * GM_PAGE_SIZE)) return GM_OVERLAP;
    s = grow(m, count);
    if (s != GM_OK) return s;
    for (size_t i = 0; i < count; ++i) {
        s = materialize(find(m, src + i * GM_PAGE_SIZE));
        if (s != GM_OK) return s;
    }
    for (size_t i = 0; i < count; ++i) {
        GmPage *p = find(m, src + i * GM_PAGE_SIZE)->page;
        ++p->refs;
        insert(m, dst + i * GM_PAGE_SIZE, p);
    }
    return GM_OK;
}
GmStatus gm_protect(GuestMemory *m, GuestAddr base, size_t size, int writable) {
    if (!aligned_range(base, size)) return GM_RANGE;
    GmStatus s = check(m, base, size, 0);
    if (s != GM_OK) return s;
    for (size_t i = 0; i < size; i += GM_PAGE_SIZE) find(m, base + i)->writable = !!writable;
    return GM_OK;
}
GmStatus gm_read(const GuestMemory *m, GuestAddr a, void *out, size_t n) {
    if (!n) return GM_OK;
    if (!out || !valid_range(a, n)) return GM_RANGE;
    size_t offset = (size_t)(a % GM_PAGE_SIZE);
    if (n <= GM_PAGE_SIZE - offset) {
        GmEntry *e = find(m, a);
        if (!e) return GM_UNMAPPED;
        if (e->page) memcpy(out, e->page->data + offset, n);
        else memset(out, 0, n);
        return GM_OK;
    }
    GmStatus s = check(m, a, n, 0);
    if (s != GM_OK) return s;
    unsigned char *p = out;
    while (n) {
        GmEntry *e = find(m, a);
        offset = (size_t)(a % GM_PAGE_SIZE);
        size_t step = GM_PAGE_SIZE - offset;
        if (step > n) step = n;
        if (e->page) memcpy(p, e->page->data + offset, step);
        else memset(p, 0, step);
        a += step; p += step; n -= step;
    }
    return GM_OK;
}
static int is_zero(const unsigned char *p, size_t n) {
    for (size_t i = 0; i < n; ++i) if (p[i]) return 0;
    return 1;
}
GmStatus gm_write(GuestMemory *m, GuestAddr a, const void *data, size_t n) {
    if (!n) return GM_OK;
    if (!data || !valid_range(a, n)) return GM_RANGE;
    size_t offset = (size_t)(a % GM_PAGE_SIZE);
    if (n <= GM_PAGE_SIZE - offset) {
        GmEntry *e = find(m, a);
        if (!e) return GM_UNMAPPED;
        if (!e->writable) return GM_READONLY;
        if (e->page || !is_zero(data, n)) {
            GmStatus s = materialize(e);
            if (s != GM_OK) return s;
            memcpy(e->page->data + offset, data, n);
            atomic_store_explicit(&e->page->dirty, 1, memory_order_release);
        }
        return GM_OK;
    }
    GmStatus s = check(m, a, n, 1); /* validate entire span before mutating */
    if (s != GM_OK) return s;
    const unsigned char *p = data, *q = p;
    GuestAddr cur = a;
    for (size_t rem = n; rem;) {
        GmEntry *e = find(m, cur);
        size_t step = GM_PAGE_SIZE - (size_t)(cur % GM_PAGE_SIZE);
        if (step > rem) step = rem;
        if (e->page || !is_zero(q, step)) {
            s = materialize(e);
            if (s != GM_OK) return s;
        }
        cur += step; q += step; rem -= step;
    }
    while (n) {
        GmEntry *e = find(m, a);
        offset = (size_t)(a % GM_PAGE_SIZE);
        size_t step = GM_PAGE_SIZE - offset;
        if (step > n) step = n;
        if (e->page) {
            memcpy(e->page->data + offset, p, step);
            atomic_store_explicit(&e->page->dirty, 1, memory_order_release);
        }
        a += step; p += step; n -= step;
    }
    return GM_OK;
}
GmStatus gm_span(GuestMemory *m, GuestAddr a, size_t n, int write, void **out) {
    if (!out) return GM_RANGE;
    *out = NULL;
    if (!n || n > GM_PAGE_SIZE - a % GM_PAGE_SIZE || !valid_range(a, n)) return GM_RANGE;
    GmEntry *e = find(m, a);
    if (!e) return GM_UNMAPPED;
    if (write && !e->writable) return GM_READONLY;
    GmStatus s = materialize(e);
    if (s != GM_OK) return s;
    if (write) atomic_store_explicit(&e->page->dirty, 1, memory_order_release);
    *out = e->page->data + a % GM_PAGE_SIZE;
    return GM_OK;
}
GmStatus gm_cas64(GuestMemory *m, GuestAddr a, uint64_t *expected, uint64_t desired, int *exchanged) {
    return gm_cas(m, a, 64, expected, desired, exchanged);
}
GmStatus gm_cas(GuestMemory *m, GuestAddr a, unsigned bits, uint64_t *expected, uint64_t desired, int *exchanged) {
    if (!expected || !exchanged) return GM_RANGE;
    *exchanged = 0;
    if (bits != 8 && bits != 16 && bits != 32 && bits != 64) return GM_RANGE;
    if (a % (bits / 8)) return GM_ALIGNMENT;
    void *p;
    GmStatus s = gm_span(m, a, bits / 8, 1, &p);
    if (s != GM_OK) return s;
#define CAS_WIDTH(width) case width: { \
        uint##width##_t e = (uint##width##_t)*expected; \
        *exchanged = __atomic_compare_exchange_n((uint##width##_t *)p, &e, (uint##width##_t)desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
        *expected = e; break; }
    switch (bits) { CAS_WIDTH(8) CAS_WIDTH(16) CAS_WIDTH(32) CAS_WIDTH(64) }
#undef CAS_WIDTH
    return GM_OK;
}
unsigned gm_dirty(const GuestMemory *m, GuestAddr a) {
    GmEntry *e = find(m, a);
    return e && e->page ? atomic_load_explicit(&e->page->dirty, memory_order_acquire) : 0;
}
void gm_clear_dirty(GuestMemory *m) {
    /* Quiescent point only: clearing concurrently can lose notifications. */
    for (size_t i = 0; i < m->page_count; ++i)
        if (m->pages[i].page) atomic_store(&m->pages[i].page->dirty, 0);
}
const char *gm_status_name(GmStatus s) {
    static const char *names[] = {"ok", "range", "unmapped", "readonly", "alignment", "capacity", "nomem", "overlap"};
    return (unsigned)s < sizeof names / sizeof *names ? names[s] : "unknown";
}

#undef gm_destroy
#undef gm_reserve
#undef gm_commit
#undef gm_decommit
#undef gm_os_commit
#undef gm_os_decommit
#undef gm_alias
#undef gm_protect
#undef gm_read
#undef gm_write
#undef gm_span
#undef gm_cas64
#undef gm_cas
#undef gm_dirty
#undef gm_clear_dirty
static void lock_map(const GuestMemory *memory) {
    GuestMemory *m = (GuestMemory *)memory;
    while (atomic_exchange_explicit(&m->map_lock, 1, memory_order_acquire))
        while (atomic_load_explicit(&m->map_lock, memory_order_relaxed)) { }
}
static void unlock_map(const GuestMemory *memory) {
    atomic_store_explicit(&((GuestMemory *)memory)->map_lock, 0, memory_order_release);
}
#define WRAP_STATUS(name, signature, args) \
    GmStatus name signature { lock_map(m); GmStatus result = name##_unlocked args; unlock_map(m); return result; }
WRAP_STATUS(gm_reserve, (GuestMemory *m, GuestAddr a, uint64_t n), (m, a, n))
WRAP_STATUS(gm_commit, (GuestMemory *m, GuestAddr a, size_t n), (m, a, n))
WRAP_STATUS(gm_decommit, (GuestMemory *m, GuestAddr a, size_t n), (m, a, n))
WRAP_STATUS(gm_os_commit, (GuestMemory *m, GuestAddr a, size_t n), (m, a, n))
WRAP_STATUS(gm_os_decommit, (GuestMemory *m, GuestAddr a, size_t n), (m, a, n))
WRAP_STATUS(gm_alias, (GuestMemory *m, GuestAddr dst, GuestAddr src, size_t n), (m, dst, src, n))
WRAP_STATUS(gm_protect, (GuestMemory *m, GuestAddr a, size_t n, int writable), (m, a, n, writable))
WRAP_STATUS(gm_read, (const GuestMemory *m, GuestAddr a, void *out, size_t n), (m, a, out, n))
WRAP_STATUS(gm_write, (GuestMemory *m, GuestAddr a, const void *data, size_t n), (m, a, data, n))
WRAP_STATUS(gm_span, (GuestMemory *m, GuestAddr a, size_t n, int write, void **out), (m, a, n, write, out))
WRAP_STATUS(gm_cas64, (GuestMemory *m, GuestAddr a, uint64_t *expected, uint64_t value, int *changed), (m, a, expected, value, changed))
WRAP_STATUS(gm_cas, (GuestMemory *m, GuestAddr a, unsigned bits, uint64_t *expected, uint64_t value, int *changed), (m, a, bits, expected, value, changed))
unsigned gm_dirty(const GuestMemory *m, GuestAddr a) { lock_map(m); unsigned value = gm_dirty_unlocked(m, a); unlock_map(m); return value; }
void gm_clear_dirty(GuestMemory *m) { lock_map(m); gm_clear_dirty_unlocked(m); unlock_map(m); }
void gm_destroy(GuestMemory *m) { lock_map(m); gm_destroy_unlocked(m); }
