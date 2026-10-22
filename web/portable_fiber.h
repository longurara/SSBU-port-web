#ifndef WEB_PORTABLE_FIBER_H
#define WEB_PORTABLE_FIBER_H
#include <emscripten/fiber.h>
#include <stdatomic.h>
/* Static-module migration adapter. No SDK files are patched. Dynamic module
 * changes are rejected until a shared export-ID protocol covers them. */
int web_portable_fiber_swap(emscripten_fiber_t *old_fiber,emscripten_fiber_t *new_fiber);
int web_portable_fiber_swap_tracked(emscripten_fiber_t *old_fiber,emscripten_fiber_t *new_fiber,atomic_int *suspended);
#endif
