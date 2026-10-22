/* libc++ unsigned std::less array specializations. Work through guest memory:
 * a Switch pointer is never cast to a Wasm pointer. Heap sort bounds stack and
 * temporary storage even for adversarially ordered input. */
#include "runtime.h"
#include <string.h>

static uint64_t value(uint64_t base,uint64_t index,unsigned width) {
    return width==4?rd32(base+index*width):rd64(base+index*width);
}
static void store(uint64_t base,uint64_t index,unsigned width,uint64_t item) {
    if(width==4)wr32(base+index*width,(uint32_t)item);else wr64(base+index*width,item);
}
static void sift(uint64_t base,uint64_t root,uint64_t count,unsigned width) {
    uint64_t item=value(base,root,width);
    while(root<count/2) {
        uint64_t child=root*2+1;
        if(child+1<count&&value(base,child,width)<value(base,child+1,width))child++;
        uint64_t larger=value(base,child,width);
        if(item>=larger)break;
        store(base,root,width,larger);root=child;
    }
    store(base,root,width,item);
}
static int less_at(uint64_t base, uint64_t i, uint64_t j, unsigned kind) {
    if (kind == 1) return rd32(base + i * 4) < rd32(base + j * 4);
    if (kind == 2) return rd64(base + i * 8) < rd64(base + j * 8);
    if (kind == 3) return (int32_t)rd32(base + i * 4) < (int32_t)rd32(base + j * 4);
    union { uint32_t u; float f; } a = {rd32(base + i * 4)}, b = {rd32(base + j * 4)};
    return a.f < b.f;
}
static void swap_at(uint64_t base, uint64_t i, uint64_t j, unsigned width) {
    uint64_t a = value(base, i, width), b = value(base, j, width);
    store(base, i, width, b); store(base, j, width, a);
}
static void sift_kind(uint64_t base, uint64_t root, uint64_t count, unsigned width, unsigned kind) {
    while (root < count / 2) {
        uint64_t child = root * 2 + 1;
        if (child + 1 < count && less_at(base, child, child + 1, kind)) child++;
        if (!less_at(base, root, child, kind)) break;
        swap_at(base, root, child, width);
        root = child;
    }
}
int web_unsigned_sort_import(Ctx *c,const char *name) {
    unsigned width = 4, kind = 0;
    if(!strcmp(name,"_ZNSt3__16__sortIRNS_6__lessIjjEEPjEEvT0_S5_T_")){width=4;kind=1;}
    else if(!strcmp(name,"_ZNSt3__16__sortIRNS_6__lessIyyEEPyEEvT0_S5_T_")){width=8;kind=2;}
    else if(!strcmp(name,"_ZNSt3__16__sortIRNS_6__lessIiiEEPiEEvT0_S5_T_")){width=4;kind=3;}
    else if(!strcmp(name,"_ZNSt3__16__sortIRNS_6__lessIffEEPfEEvT0_S5_T_")){width=4;kind=4;}
    else return 0;
    uint64_t base=c->x[0],end=c->x[1];
    if(end<base||(end-base)%width)guest_trap("invalid unsigned sort range",base);
    uint64_t count=(end-base)/width;
    if(count<2)return 1;
    if(kind<=2){
        for(uint64_t root=count/2;root;)sift(base,--root,count,width);
        for(uint64_t remaining=count;remaining>1;) {
            uint64_t top=value(base,0,width);remaining--;
            store(base,0,width,value(base,remaining,width));store(base,remaining,width,top);
            sift(base,0,remaining,width);
        }
    } else {
        for(uint64_t root=count/2;root;)sift_kind(base,--root,count,width,kind);
        for(uint64_t remaining=count;remaining>1;) {
            remaining--;swap_at(base,0,remaining,width);
            sift_kind(base,0,remaining,width,kind);
        }
    }
    return 1;
}
