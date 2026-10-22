#ifndef WEB_GUEST_MEMORY_H
#define WEB_GUEST_MEMORY_H

/* Guest address adapter, used by the opt-in runtime memory helpers.
 * Mapping/protection changes and adapter reads/writes serialize on map_lock.
 * Destroy requires all accessing workers to stop. Borrowed spans require the
 * caller to prevent changes to their backing lifetime and concurrent byte
 * access. Use gm_cas64 for atomic read-modify-write. No executable mappings. */
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

#define GM_PAGE_SIZE 4096u
#define GM_MAX_PAGES 256u
#define GM_HARD_MAX_PAGES 1048576u
#define GM_MAX_REGIONS 16u
typedef uint64_t GuestAddr;
typedef enum { GM_OK, GM_RANGE, GM_UNMAPPED, GM_READONLY, GM_ALIGNMENT,
               GM_CAPACITY, GM_NOMEM, GM_OVERLAP } GmStatus;
typedef struct GmPage GmPage;
typedef struct { GuestAddr base; uint64_t size; } GmRegion;
typedef struct { GuestAddr base; GmPage *page; int writable; } GmEntry;
typedef struct {
    GmRegion regions[GM_MAX_REGIONS];
    GmEntry *pages; /* sorted by guest page base; growth guarded by map_lock */
    size_t region_count, page_count, backing_pages;
    size_t capacity, page_limit;
    atomic_uint map_lock;
} GuestMemory;

void gm_init(GuestMemory *m);
/* Initialise a new/uninitialised object; do not call on a live mapping. */
GmStatus gm_init_limit(GuestMemory *m, size_t page_limit);
void gm_destroy(GuestMemory *m);
GmStatus gm_reserve(GuestMemory *m, GuestAddr base, uint64_t size);
GmStatus gm_commit(GuestMemory *m, GuestAddr base, size_t size);
GmStatus gm_decommit(GuestMemory *m, GuestAddr base, size_t size);
GmStatus gm_alias(GuestMemory *m, GuestAddr dst, GuestAddr src, size_t size);
GmStatus gm_protect(GuestMemory *m, GuestAddr base, size_t size, int writable);
/* Host input/output spans must not overlap this adapter's backing storage. */
GmStatus gm_read(const GuestMemory *m, GuestAddr a, void *out, size_t n);
GmStatus gm_write(GuestMemory *m, GuestAddr a, const void *data, size_t n);
/* Returns a borrowed pointer only for a single mapped page; caller cannot retain
 * it through decommit/destroy. A writable span marks dirty before it is returned. */
GmStatus gm_span(GuestMemory *m, GuestAddr a, size_t n, int write, void **out);
GmStatus gm_cas64(GuestMemory *m, GuestAddr a, uint64_t *expected,
                  uint64_t desired, int *exchanged);
GmStatus gm_cas(GuestMemory *m, GuestAddr a, unsigned bits,
               uint64_t *expected, uint64_t desired, int *exchanged);
unsigned gm_dirty(const GuestMemory *m, GuestAddr a);
void gm_clear_dirty(GuestMemory *m);
const char *gm_status_name(GmStatus s);
#endif
