/* Deliberately separate from the switching function: re-read the destination
 * Worker's TLS instead of carrying a cached source-thread TLS pointer. */
#include <pthread.h>
#include <stdint.h>
#include <emscripten/fiber.h>
static _Thread_local int value;
static _Thread_local emscripten_fiber_t *root;
void migration_set_tls(int next){value=next;}
int migration_get_tls(void){return value;}
uintptr_t migration_native_thread(void){return (uintptr_t)pthread_self();}
void migration_set_root(emscripten_fiber_t *next){root=next;}
emscripten_fiber_t *migration_get_root(void){return root;}
