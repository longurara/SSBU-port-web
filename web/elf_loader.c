/* Minimal ELF64 image loading for the translated-code runtime. Guest pointers
 * remain virtual addresses; no host pointer casts. No relocation or execution. */
#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64
#include "guest_memory.h"
#include <stdio.h>
#include <string.h>

#define IMAGE_BASE 0x7100000000ULL
#define IMAGE_LIMIT (512ULL * 1024 * 1024)
typedef struct {
    unsigned char ident[16];
    uint16_t type, machine;
    uint32_t version;
    uint64_t entry, phoff, shoff;
    uint32_t flags;
    uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
} ElfHeader;
typedef struct {
    uint32_t type, flags;
    uint64_t offset, vaddr, paddr, filesz, memsz, align;
} ProgramHeader;
_Static_assert(sizeof(ElfHeader) == 64 && sizeof(ProgramHeader) == 56, "ELF layout");
static GuestMemory image;
static int loaded;
static char info[512];
static char relocation_info[512];
#ifdef WEB_FULL_RUNTIME
uint64_t runtime_import_address(unsigned symbol);
#endif
static uint64_t dynamic_address, dynamic_size;
static unsigned char chunk[65536];
GuestMemory *elf_memory(void) { return loaded ? &image : NULL; }

void elf_unload(void) {
    if (loaded) gm_destroy(&image);
    loaded = 0;
    dynamic_address = dynamic_size = 0;
}
const char *elf_info(void) { return info; }
static int fail(FILE *file, const char *message) {
    if (file) fclose(file);
    elf_unload();
    snprintf(info, sizeof info, "{\"ok\":false,\"error\":\"%s\"}", message);
    return 0;
}
int elf_load(const char *path) {
    elf_unload();
    FILE *file = fopen(path, "rb");
    if (!file) return fail(NULL, "Cannot open ELF");
    if (fseeko(file, 0, SEEK_END) != 0) return fail(file, "Cannot seek ELF");
    off_t file_size = ftello(file);
    ElfHeader header;
    if (file_size < (off_t)sizeof header || fseeko(file, 0, SEEK_SET) || fread(&header, 1, sizeof header, file) != sizeof header)
        return fail(file, "Truncated ELF header");
    if (memcmp(header.ident, "\177ELF\2\1\1", 7) || header.machine != 183 || header.version != 1 ||
        header.ehsize != sizeof header || header.phentsize != sizeof(ProgramHeader) || !header.phnum || header.phnum > 16)
        return fail(file, "Expected little-endian ELF64 AArch64");
    if (header.phoff > (uint64_t)file_size || header.phnum * sizeof(ProgramHeader) > (uint64_t)file_size - header.phoff)
        return fail(file, "Invalid program header range");
    ProgramHeader programs[16];
    if (fseeko(file, (off_t)header.phoff, SEEK_SET) || fread(programs, sizeof(ProgramHeader), header.phnum, file) != header.phnum)
        return fail(file, "Cannot read program headers");
    if (gm_init_limit(&image, GM_HARD_MAX_PAGES) != GM_OK) return fail(file, "Memory initialisation failed");
    loaded = 1;
    if (gm_reserve(&image, IMAGE_BASE, IMAGE_LIMIT) != GM_OK) return fail(file, "Image reservation failed");
    unsigned segments = 0;
    uint64_t file_bytes = 0, bss_bytes = 0, first_readonly = 0;
    for (unsigned i = 0; i < header.phnum; i++) {
        ProgramHeader *p = &programs[i];
        if (p->type == 2) {
            if (p->vaddr >= IMAGE_LIMIT || p->filesz > IMAGE_LIMIT - p->vaddr || p->filesz > 65536)
                return fail(file, "Invalid dynamic segment");
            dynamic_address = p->vaddr; dynamic_size = p->filesz;
        }
        if (p->type != 1 || !p->memsz) continue;
        if ((p->vaddr & (GM_PAGE_SIZE - 1)) || p->vaddr >= IMAGE_LIMIT || p->memsz > IMAGE_LIMIT - p->vaddr ||
            p->filesz > p->memsz || p->offset > (uint64_t)file_size || p->filesz > (uint64_t)file_size - p->offset)
            return fail(file, "Invalid LOAD segment bounds or alignment");
        uint64_t address = IMAGE_BASE + p->vaddr;
        size_t mapped = (size_t)((p->memsz + GM_PAGE_SIZE - 1) & ~(uint64_t)(GM_PAGE_SIZE - 1));
        if (gm_commit(&image, address, mapped) != GM_OK) return fail(file, "Cannot map LOAD segment");
        if (fseeko(file, (off_t)p->offset, SEEK_SET)) return fail(file, "Cannot seek LOAD segment");
        for (uint64_t offset = 0; offset < p->filesz;) {
            size_t n = p->filesz - offset < sizeof chunk ? (size_t)(p->filesz - offset) : sizeof chunk;
            if (fread(chunk, 1, n, file) != n || gm_write(&image, address + offset, chunk, n) != GM_OK)
                return fail(file, "Cannot copy LOAD segment");
            offset += n;
        }
        // calloc-backed pages must leave every BSS byte zero, including a partial page.
        for (uint64_t offset = p->filesz; offset < p->memsz;) {
            size_t n = p->memsz - offset < sizeof chunk ? (size_t)(p->memsz - offset) : sizeof chunk;
            if (gm_read(&image, address + offset, chunk, n) != GM_OK) return fail(file, "Cannot read BSS");
            for (size_t j = 0; j < n; j++) if (chunk[j]) return fail(file, "BSS is not zero");
            offset += n;
        }
        if (!(p->flags & 2)) {
            if (gm_protect(&image, address, mapped, 0) != GM_OK) return fail(file, "Cannot protect segment");
            first_readonly = address;
        }
        file_bytes += p->filesz; bss_bytes += p->memsz - p->filesz; segments++;
    }
    fclose(file);
    if (!segments) return fail(NULL, "No LOAD segments");
    int protected = first_readonly && gm_write(&image, first_readonly, chunk, 1) == GM_READONLY;
    snprintf(info, sizeof info, "{\"ok\":true,\"segments\":%u,\"fileBytes\":%llu,\"bssBytes\":%llu,\"pages\":%zu,\"readonlyEnforced\":%s,\"relocationsApplied\":false,\"nnMainExecuted\":false}",
             segments, (unsigned long long)file_bytes, (unsigned long long)bss_bytes, image.page_count, protected ? "true" : "false");
    return 1;
}
uint32_t elf_hash(uint64_t relative, uint32_t size) {
    if (!loaded || relative >= IMAGE_LIMIT || !size || size > sizeof chunk || size > IMAGE_LIMIT - relative ||
        gm_read(&image, IMAGE_BASE + relative, chunk, size) != GM_OK) return 0;
    uint32_t hash = 2166136261u;
    for (uint32_t i = 0; i < size; i++) hash = (hash ^ chunk[i]) * 16777619u;
    return hash;
}
uint64_t elf_word(uint64_t relative) {
    uint64_t value = 0;
    if (loaded && relative <= IMAGE_LIMIT - 8) gm_read(&image, IMAGE_BASE + relative, &value, 8);
    return value;
}
const char *elf_relocation_info(void) { return relocation_info; }
static int relocation_fail(const char *message) {
    elf_unload();
    snprintf(relocation_info, sizeof relocation_info, "{\"ok\":false,\"error\":\"%s\"}", message);
    return 0;
}
static int image_read(uint64_t offset, void *out, size_t size) {
    return loaded && offset < IMAGE_LIMIT && size <= IMAGE_LIMIT - offset &&
           gm_read(&image, IMAGE_BASE + offset, out, size) == GM_OK;
}
int elf_relocate(void) {
    if (!loaded || !dynamic_address || !dynamic_size || dynamic_size % 16)
        return relocation_fail("Missing or invalid ELF dynamic table");
    uint64_t tags[24] = {0};
    int ended = 0;
    for (uint64_t i = 0; i < dynamic_size; i += 16) {
        uint64_t pair[2];
        if (!image_read(dynamic_address + i, pair, sizeof pair)) return relocation_fail("Dynamic table outside image");
        if (!pair[0]) { ended = 1; break; }
        if (pair[0] < 24) tags[pair[0]] = pair[1];
    }
    if (!ended || !tags[6] || tags[11] != 24 || tags[9] != 24 || (tags[2] && tags[20] != 7))
        return relocation_fail("Unsupported dynamic table layout");
    const uint64_t tables[2][2] = {{tags[7], tags[8]}, {tags[23], tags[2]}};
    unsigned relative = 0, own = 0, imports = 0, weak = 0;
    for (unsigned table = 0; table < 2; table++) {
        uint64_t address = tables[table][0], size = tables[table][1];
        if (size % 24 || size / 24 > 1000000 || address >= IMAGE_LIMIT || size > IMAGE_LIMIT - address)
            return relocation_fail("Invalid relocation table bounds");
        for (uint64_t i = 0; i < size; i += 24) {
            uint64_t row[3], value;
            if (!image_read(address + i, row, sizeof row)) return relocation_fail("Cannot read relocation");
            uint32_t type = (uint32_t)row[1], symbol = (uint32_t)(row[1] >> 32);
            if (type == 1027) { value = IMAGE_BASE + row[2]; relative++; }
            else if (type == 257 || type == 1025 || type == 1026) {
                unsigned char sym[24];
                if (symbol >= 65536 || !image_read(tags[6] + (uint64_t)symbol * 24, sym, sizeof sym))
                    return relocation_fail("Invalid relocation symbol");
                uint16_t defined; uint64_t symbol_value;
                memcpy(&defined, sym + 6, 2); memcpy(&symbol_value, sym + 8, 8);
                if (defined) { value = IMAGE_BASE + symbol_value; own++; }
                else if ((sym[4] >> 4) == 2) { value = 0; weak++; }
                else {
                    // Symbolic slots are addresses, not implemented SDK services.
                    // They remain unmapped until a real import binding exists.
                    value = 0x7400000000ULL + (uint64_t)symbol * 256; imports++;
#ifdef WEB_FULL_RUNTIME
                    uint64_t bound = runtime_import_address(symbol);
                    if (bound) value = bound;
#endif
                }
                if (type == 257) value += row[2];
            } else return relocation_fail("Unsupported relocation type");
            if (row[0] > IMAGE_LIMIT - 8 || gm_write(&image, IMAGE_BASE + row[0], &value, 8) != GM_OK)
                return relocation_fail("Relocation target is unmapped or read-only");
        }
    }
    snprintf(relocation_info, sizeof relocation_info, "{\"ok\":true,\"relative\":%u,\"own\":%u,\"imports\":%u,\"weak\":%u,\"importsImplemented\":false}", relative, own, imports, weak);
    return 1;
}
