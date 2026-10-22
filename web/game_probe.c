/* Execute unmodified locally lifted leaf functions against independent AArch64
 * register and whole-data-window oracles. No game bytes live in this source. */
#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>

#define DATA 0x7200300000ULL
#define DATA_SIZE 65536u
typedef struct { unsigned offset; unsigned char value; } MemoryChange;
typedef struct {
    unsigned function, seed, flags, out_flags;
    uint64_t x[32], vectors[64], out_x[32], out_vectors[64];
    const MemoryChange *changes;
    size_t change_count;
} GameCase;
#include "game_vectors.h"

static unsigned char initial[DATA_SIZE], expected[DATA_SIZE], actual[DATA_SIZE];
void guest_trap(const char *what, uint64_t address) {
    fprintf(stderr, "game trap %s at %llx\n", what, (unsigned long long)address); abort();
}
void guest_unimpl(uint64_t pc, uint32_t instruction, const char *what) {
    (void)instruction; guest_trap(what, pc);
}
void call_indirect(Ctx *c, uint64_t address) { (void)c; guest_trap("unexpected indirect call", address); }
void host_import(Ctx *c, int symbol) { (void)c; guest_trap("unexpected import", (uint64_t)symbol); }
void guest_svc(Ctx *c, uint32_t number) { (void)c; guest_trap("unexpected svc", number); }
void stx_diag(Ctx *c, uint64_t a, uint64_t wanted, uint64_t seen, uint64_t value, int bits) {
    (void)c; (void)a; (void)wanted; (void)seen; (void)value; (void)bits;
}

int main(void) {
    GuestMemory memory;
    gm_init(&memory);
    if (gm_reserve(&memory, DATA, DATA_SIZE) != GM_OK || gm_commit(&memory, DATA, DATA_SIZE) != GM_OK) return 2;
    web_memory_bind(&memory);
    unsigned failures = 0;
    const unsigned count = sizeof(game_cases) / sizeof(game_cases[0]);
    for (unsigned i = 0; i < count; i++) {
        const GameCase *test = &game_cases[i];
        Ctx c = {0};
        memcpy(c.x, test->x, sizeof c.x);
        memcpy(c.v, test->vectors, sizeof c.v);
        c.N = (test->flags >> 3) & 1; c.Z = (test->flags >> 2) & 1;
        c.C = (test->flags >> 1) & 1; c.V = test->flags & 1;
        for (unsigned j = 0; j < DATA_SIZE; j++) initial[j] = (unsigned char)(j * 17u + (j >> 8) * 13u + test->seed * 29u);
        memcpy(expected, initial, DATA_SIZE);
        for (size_t j = 0; j < test->change_count; j++) expected[test->changes[j].offset] = test->changes[j].value;
        if (gm_write(&memory, DATA, initial, DATA_SIZE) != GM_OK) return 2;
        game_functions[test->function](&c);
        if (gm_read(&memory, DATA, actual, DATA_SIZE) != GM_OK) return 2;
        unsigned flags = (c.N << 3) | (c.Z << 2) | (c.C << 1) | c.V;
        int registers = memcmp(c.x, test->out_x, sizeof c.x) == 0;
        int vectors = memcmp(c.v, test->out_vectors, sizeof c.v) == 0;
        int bytes = memcmp(actual, expected, DATA_SIZE) == 0;
        int ok = registers && vectors && bytes && flags == test->out_flags;
        failures += !ok;
        printf("SSBU_CHECK {\"name\":\"game/%llx/seed%u\",\"ok\":%s,\"registers\":%d,\"vectors\":%d,\"memory\":%d,\"flags\":%d}\n",
               (unsigned long long)game_addresses[test->function], test->seed, ok ? "true" : "false", registers, vectors, bytes, flags == test->out_flags);
    }
    web_memory_bind(NULL);
    gm_destroy(&memory);
    printf("SSBU_GAME_RESULT {\"checks\":%u,\"failures\":%u,\"functions\":%u,\"pointerBits\":%u}\n",
           count, failures, (unsigned)(sizeof(game_functions) / sizeof(game_functions[0])), (unsigned)(sizeof(void *) * 8));
    return failures ? 1 : 0;
}
