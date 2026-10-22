/* Same-thread stackful fiber probe. Migration is deliberately NOT attempted:
 * Emscripten's Asyncify fiber API documents that as unsupported. */
#include <emscripten/fiber.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

static emscripten_fiber_t caller, task;
static int stage, sum, failures;
static void nested(unsigned depth) {
    volatile unsigned saved = depth + 100;
    if (depth) nested(depth - 1);
    else {
        jmp_buf cancel;
        if (!setjmp(cancel)) {
            sum = 41; stage = 1;
            emscripten_fiber_swap(&task, &caller);
            if (sum != 41 || stage != 2) ++failures;
            sum++;
            longjmp(cancel, 1);
        } else if (sum != 42) ++failures;
    }
    if (saved != depth + 100) ++failures;
}
static void entry(void *unused) {
    (void)unused;
    nested(4);
    stage = 3;
    emscripten_fiber_swap(&task, &caller);
    abort();
}
int main(void) {
    void *stack = aligned_alloc(16,65536), *async_task = aligned_alloc(16,65536), *async_caller = aligned_alloc(16,65536);
    if (!stack || !async_task || !async_caller) {
        puts("SSBU_FIBER_RESULT {\"failures\":1,\"reason\":\"allocation\"}"); return 1;
    }
    emscripten_fiber_init_from_current_context(&caller, async_caller, 65536);
    emscripten_fiber_init(&task, entry, NULL, stack, 65536, async_task, 65536);
    emscripten_fiber_swap(&caller, &task);
    if (stage != 1 || sum != 41) ++failures;
    stage = 2;
    emscripten_fiber_swap(&caller, &task);
    if (stage != 3 || sum != 42) ++failures;
    printf("SSBU_FIBER_RESULT {\"failures\":%d,\"pointerBits\":%zu,\"nestedDepth\":4,\"sameThreadOnly\":true,\"migration\":\"unsupported by this API; explicit continuation fixture is separate\"}\n", failures, sizeof(void *) * 8);
    /* task remains suspended; its stacks stay alive until this module is discarded. */
    return failures ? 1 : 0;
}
