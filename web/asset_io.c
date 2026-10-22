/* Worker-local, read-only file I/O. off_t must stay 64-bit on wasm32. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

_Static_assert(sizeof(off_t) == 8, "RomFS requires 64-bit file offsets");
static unsigned char buffer[65536];
static int last_status;

int asset_status(void) { return last_status; }
int64_t asset_size(const char *path) {
    last_status = 0;
    FILE *file = fopen(path, "rb");
    if (!file) { last_status = 1; return -1; }
    int64_t size = -1;
    if (fseeko(file, 0, SEEK_END) == 0) size = (int64_t)ftello(file);
    if (size < 0) last_status = 2;
    fclose(file);
    return size;
}

uint32_t asset_hash(const char *path, uint64_t offset, uint32_t length) {
    last_status = 0;
    if (offset > INT64_MAX || length > 1048576) { last_status = 3; return 0; }
    FILE *file = fopen(path, "rb");
    if (!file) { last_status = 1; return 0; }
    if (fseeko(file, (off_t)offset, SEEK_SET) != 0) {
        fclose(file); last_status = 2; return 0;
    }
    uint32_t hash = 2166136261u;
    while (length) {
        size_t wanted = length < sizeof buffer ? length : sizeof buffer;
        size_t got = fread(buffer, 1, wanted, file);
        if (got != wanted) { last_status = 4; break; }
        for (size_t i = 0; i < got; i++) hash = (hash ^ buffer[i]) * 16777619u;
        length -= (uint32_t)got;
    }
    fclose(file);
    return hash;
}
