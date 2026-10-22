/* Compile-only compatibility check for the unchanged default helper branch.
 * Not a test of the full Windows runtime or a native game build. */
#include "runtime.h"
void native_header_check(Ctx *c, uint64_t address) {
    wr8(address, rd8(address));
    wr16(address, rd16(address));
    wr32(address, rd32(address));
    wr64(address, rd64(address));
    V128 v = rdv(address, 16); wrv(address, &v, 16);
    c->x[0] = stx64(c, address, c->x[0]);
}
