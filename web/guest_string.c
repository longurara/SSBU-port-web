/* libc++ AArch64 string ABI, using the game's own buffer allocator. Inlined
 * guest code may free these buffers, so ordinary host malloc is not compatible. */
#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_long(uint64_t object) { return rd8(object) & 1; }
static uint64_t data_at(uint64_t object) { return is_long(object) ? rd64(object + 16) : object + 1; }
static uint64_t length_at(uint64_t object) { return is_long(object) ? rd64(object + 8) : rd8(object) >> 1; }
static uint64_t capacity_at(uint64_t object) { return is_long(object) ? (rd64(object) & ~1ULL) - 1 : 22; }
static uint64_t c_length(uint64_t address) { uint64_t size = 0; while (rd8(address + size)) size++; return size; }
static void set_length(uint64_t object, uint64_t size) {
    if (is_long(object)) wr64(object + 8, size); else wr8(object, (uint8_t)(size << 1));
}
static uint64_t allocate(Ctx *c, uint64_t size) {
    uint64_t save[] = {c->x[0], c->x[1], c->x[2], c->x[30]};
    c->x[0] = 16; c->x[1] = size;
    call_indirect(c, 0x710392dce0ULL);
    uint64_t pointer = c->x[0];
    c->x[0] = save[0]; c->x[1] = save[1]; c->x[2] = save[2]; c->x[30] = save[3];
    if (!pointer) guest_trap("game string allocation returned null", size);
    return pointer;
}
static void release(Ctx *c, uint64_t pointer) {
    uint64_t save[] = {c->x[0], c->x[1], c->x[2], c->x[30]};
    c->x[0] = pointer; call_indirect(c, 0x710392e590ULL);
    c->x[0] = save[0]; c->x[1] = save[1]; c->x[2] = save[2]; c->x[30] = save[3];
}
static void reserve(Ctx *c, uint64_t object, uint64_t need) {
    uint64_t capacity = capacity_at(object), size = length_at(object);
    if (need <= capacity) return;
    if (need > SIZE_MAX - 16 || capacity > (SIZE_MAX - 16) / 2) guest_trap("string capacity overflow", object);
    uint64_t want = need > capacity * 2 ? need : capacity * 2;
    uint64_t allocated = (want + 16) & ~15ULL, pointer = allocate(c, allocated);
    if (size > SIZE_MAX - 1) guest_trap("string length overflow", object);
    unsigned char *copy = malloc((size_t)size + 1);
    if (!copy) guest_trap("string copy allocation failed", object);
    web_read(data_at(object), copy, (size_t)size + 1); web_write(pointer, copy, (size_t)size + 1); free(copy);
    if (is_long(object)) release(c, data_at(object));
    wr64(object, allocated | 1); wr64(object + 8, size); wr64(object + 16, pointer);
}
static void assign(Ctx *c, uint64_t object, uint64_t source, uint64_t size, int append) {
    uint64_t old_size = append ? length_at(object) : 0;
    if (size > SIZE_MAX || old_size > SIZE_MAX - size) guest_trap("string size overflow", object);
    unsigned char *copy = malloc(size ? (size_t)size : 1);
    if (!copy) guest_trap("string input copy allocation failed", object);
    web_read(source, copy, (size_t)size);
    reserve(c, object, old_size + size);
    uint64_t target = data_at(object);
    web_write(target + old_size, copy, (size_t)size); free(copy);
    wr8(target + old_size + size, 0); set_length(object, old_size + size);
}
static void assign_host(Ctx *c, uint64_t object, const char *s, size_t size) {
    zero_object_raw: ;
    const unsigned char zero[24] = {0};
    web_write(object, zero, sizeof zero);
    reserve(c, object, size);
    uint64_t target = data_at(object);
    web_write(target, s, size);
    wr8(target + size, 0);
    set_length(object, size);
}
static void zero_object(uint64_t object) { const unsigned char zero[24] = {0}; web_write(object, zero, sizeof zero); }
int web_string_import(Ctx *c, const char *name) {
    if (!strcmp(name, "_ZNSt3__19to_stringEi") || !strcmp(name, "_ZNSt3__19to_stringEj")) {
        char buf[32];
        int n = !strcmp(name, "_ZNSt3__19to_stringEi")
            ? snprintf(buf, sizeof buf, "%d", (int32_t)c->x[0])
            : snprintf(buf, sizeof buf, "%u", (uint32_t)c->x[0]);
        if (n < 0) n = 0;
        assign_host(c, c->x[8], buf, (size_t)n);
        c->x[0] = c->x[8];
        return 1;
    }
    if (!strcmp(name, "_ZNSt3__1plIcNS_11char_traitsIcEENS_9allocatorIcEEEENS_12basic_stringIT_T0_T1_EEPKS6_RKS9_")) {
        uint64_t out = c->x[8], lhs = c->x[0], rhs = c->x[1];
        zero_object(out);
        assign(c, out, lhs, c_length(lhs), 0);
        assign(c, out, data_at(rhs), length_at(rhs), 1);
        c->x[0] = out; return 1;
    }
    if (!strcmp(name, "_ZNSt3__1plIcNS_11char_traitsIcEENS_9allocatorIcEEEENS_12basic_stringIT_T0_T1_EERKS9_PKS6_")) {
        uint64_t out = c->x[8], lhs = c->x[0], rhs = c->x[1];
        zero_object(out);
        assign(c, out, data_at(lhs), length_at(lhs), 0);
        assign(c, out, rhs, c_length(rhs), 1);
        c->x[0] = out; return 1;
    }
    static const char cprefix[] = "_ZNKSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE";
    if (!strncmp(name, cprefix, sizeof cprefix - 1)) {
        const char *cmethod = name + sizeof cprefix - 1;
        uint64_t object = c->x[0], data = data_at(object), size = length_at(object);
        if (!strcmp(cmethod, "4findEcm")) {
            uint8_t ch = (uint8_t)c->x[1]; uint64_t pos = c->x[2];
            for (uint64_t i = pos; i < size; i++) if (rd8(data + i) == ch) { c->x[0] = i; return 1; }
            c->x[0] = ~0ULL; return 1;
        }
        if (!strcmp(cmethod, "5rfindEcm")) {
            uint8_t ch = (uint8_t)c->x[1]; uint64_t pos = c->x[2];
            if (!size) { c->x[0] = ~0ULL; return 1; }
            if (pos >= size) pos = size - 1;
            for (uint64_t i = pos + 1; i-- > 0;) if (rd8(data + i) == ch) { c->x[0] = i; return 1; }
            c->x[0] = ~0ULL; return 1;
        }
        if (!strcmp(cmethod, "7compareEmmPKcm")) {
            uint64_t pos1 = c->x[1], n1 = c->x[2], s = c->x[3], n2 = c->x[4];
            if (pos1 > size) pos1 = size;
            if (n1 > size - pos1) n1 = size - pos1;
            uint64_t m = n1 < n2 ? n1 : n2;
            for (uint64_t i = 0; i < m; i++) {
                int d = (int)rd8(data + pos1 + i) - (int)rd8(s + i);
                if (d) { c->x[0] = (uint64_t)(int64_t)d; return 1; }
            }
            c->x[0] = n1 < n2 ? (uint64_t)-1LL : (n1 > n2 ? 1ULL : 0ULL); return 1;
        }
        return 0;
    }
    if (!strcmp(name, "_ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEED1Ev") ||
        !strcmp(name, "_ZNSt3__112basic_stringIwNS_11char_traitsIwEENS_9allocatorIwEEED2Ev")) {
        uint64_t object = c->x[0];
        if (is_long(object)) release(c, data_at(object));
        return 1;
    }
    static const char prefix[] = "_ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE";
    if (strncmp(name, prefix, sizeof prefix - 1)) return 0;
    const char *method = name + sizeof prefix - 1;
    uint64_t object = c->x[0];
    if (!strcmp(method, "6assignEPKc")) assign(c, object, c->x[1], c_length(c->x[1]), 0);
    else if (!strcmp(method, "6assignEPKcm")) assign(c, object, c->x[1], c->x[2], 0);
    else if (!strcmp(method, "6appendEPKc")) assign(c, object, c->x[1], c_length(c->x[1]), 1);
    else if (!strcmp(method, "6appendEPKcm")) assign(c, object, c->x[1], c->x[2], 1);
    else if (!strcmp(method, "7reserveEm")) reserve(c, object, c->x[1]);
    else if (!strcmp(method, "C1Ev") || !strcmp(method, "C2Ev")) zero_object(object);
    else if (!strcmp(method, "D1Ev") || !strcmp(method, "D2Ev")) { if (is_long(object)) release(c, data_at(object)); }
    else if (!strcmp(method, "C1ERKS5_")) { uint64_t source = data_at(c->x[1]), size = length_at(c->x[1]); zero_object(object); assign(c, object, source, size, 0); }
    else if (!strcmp(method, "C1ERKS5_mmRKS4_") || !strcmp(method, "C2ERKS5_mmRKS4_")) {
        uint64_t size=length_at(c->x[1]),position=c->x[2],count=c->x[3];
        if(position>size)guest_trap("string substring out of range; exception bridge required",object);
        if(count>size-position)count=size-position;
        uint64_t source=data_at(c->x[1])+position;
        zero_object(object);assign(c,object,source,count,0);
    }
    else if (!strcmp(method, "aSERKS5_")) { if (object != c->x[1]) assign(c, object, data_at(c->x[1]), length_at(c->x[1]), 0); }
    else if (!strcmp(method, "9push_backEc")) {
        uint64_t size = length_at(object); reserve(c, object, size + 1); uint64_t target = data_at(object);
        wr8(target + size, (uint8_t)c->x[1]); wr8(target + size + 1, 0); set_length(object, size + 1);
    } else if (!strcmp(method, "5eraseEmm")) {
        uint64_t size = length_at(object), position = c->x[1], count = c->x[2];
        if (position > size) guest_trap("string erase out of range", object);
        if (count > size - position) count = size - position;
        uint64_t target = data_at(object), remaining = size - position - count + 1;
        unsigned char *copy = malloc((size_t)remaining); if (!copy) guest_trap("string erase copy failed", object);
        web_read(target + position + count, copy, (size_t)remaining); web_write(target + position, copy, (size_t)remaining); free(copy);
        set_length(object, size - count);
    } else if (!strcmp(method, "6resizeEmc")) {
        uint64_t n = c->x[1], old = length_at(object); uint8_t ch = (uint8_t)c->x[2];
        reserve(c, object, n); uint64_t target = data_at(object);
        for (uint64_t i = old; i < n; i++) wr8(target + i, ch);
        wr8(target + n, 0); set_length(object, n);
    } else if (!strcmp(method, "6insertEmPKc")) {
        uint64_t pos = c->x[1], s = c->x[2], slen = c_length(s), old = length_at(object);
        if (pos > old) pos = old;
        unsigned char *in_copy = malloc(slen ? (size_t)slen : 1);
        unsigned char *tail_copy = malloc((size_t)(old - pos + 1));
        if (!in_copy || !tail_copy) guest_trap("string insert copy failed", object);
        web_read(s, in_copy, (size_t)slen);
        web_read(data_at(object) + pos, tail_copy, (size_t)(old - pos + 1));
        reserve(c, object, old + slen);
        uint64_t target = data_at(object);
        web_write(target + pos + slen, tail_copy, (size_t)(old - pos + 1));
        web_write(target + pos, in_copy, (size_t)slen);
        free(in_copy); free(tail_copy);
        set_length(object, old + slen); c->x[0] = object;
    } else if (!strcmp(method, "2atEm")) {
        c->x[0] = data_at(object) + c->x[1];
    } else if (!strcmp(method, "9__grow_byEmmmmmm")) {
        uint64_t old_cap = c->x[1], delta_cap = c->x[2], old_sz = c->x[3];
        uint64_t n_copy = c->x[4], n_del = c->x[5], n_add = c->x[6];
        uint64_t alloc = (old_cap + delta_cap + 17ULL) & ~15ULL;
        uint64_t old_data = data_at(object);
        int was_long = is_long(object);
        uint64_t tail_len = old_sz > n_copy + n_del ? old_sz - n_copy - n_del : 0;
        unsigned char *head = n_copy ? malloc((size_t)n_copy) : NULL;
        unsigned char *tail = tail_len ? malloc((size_t)tail_len) : NULL;
        if (n_copy) web_read(old_data, head, (size_t)n_copy);
        if (tail_len) web_read(old_data + n_copy + n_del, tail, (size_t)tail_len);
        uint64_t p = allocate(c, alloc);
        if (n_copy) { web_write(p, head, (size_t)n_copy); free(head); }
        if (tail_len) { web_write(p + n_copy + n_add, tail, (size_t)tail_len); free(tail); }
        if (was_long) release(c, old_data);
        wr64(object, alloc | 1ULL);
        wr64(object + 16, p);
    } else return 0;
    return 1;
}
