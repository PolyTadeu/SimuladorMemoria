#ifndef __PAGE_TABLE__
#define __PAGE_TABLE__

#include "premissas.h"

typedef u8 FrameIdx;

#define Is_Flag_Set(flag, thing)    (((thing) & (flag)) == (flag))
#define Set_Flag(flag, thing)       ((thing) |= (flag))
#define Unset_Flag(flag, thing)     (((thing) &= ~(flag)))
#define Flag_Setted(flag, thing)    ((thing) | (flag))
#define Flag_Unsetted(flag, thing)  ((thing) & (~(flag)))
#define GetPageFlags(vt, addr)      (vt)->page[VirtPage(addr)].flags

typedef enum _PageFlagsEnum {
    PageFlag_Loaded      = 0x01,
    PageFlag_Modified    = 0x02,
    PageFlag_Unused04    = 0x04,
    PageFlag_Unused08    = 0x08,
    PageFlag_Unused10    = 0x10,
    PageFlag_Unused20    = 0x20,
    PageFlag_Unused40    = 0x40,
    PageFlag_Unused80    = 0x80,
} PageFlagsEnum;

typedef u8 PageFlags;

typedef struct _PageLine {
    PageFlags flags;
    FrameIdx frame;
} PageLine;

typedef struct _PageTable {
    PageLine page[MAX_PAGE];
} PageTable;

b32 isLoaded(PageTable *vtable, Vaddr addr) {
    return Is_Flag_Set(PageFlag_Loaded,
            GetPageFlags(vtable, addr));
}

void loadPage(PageTable *vtable, Vaddr addr) {
    (void) vtable; (void) addr;
    assert(0 && "loadPage is not implemented");
}

void markPageUsed(PageTable *vtable, Vaddr addr) {
    assert(isLoaded(vtable, addr)
            && "Trying to makePageUsed a unloaded Page");
    // TODO: provav aqui vai fazer alguma coisa para manter o LRU
}

void markPageModified(PageTable *vtable, Vaddr addr) {
    markPageUsed(vtable, addr);
    Set_Flag(PageFlag_Modified, GetPageFlags(vtable, addr));
}

FrameIdx getFrameIdx(PageTable *vtable, Vaddr addr) {
    assert(isLoaded(vtable, addr)
            && "Trying to getFrameIdx from a unloaded Page");
    return vtable->page[VirtPage(addr)].frame;
}

#undef Is_Flag_Set
#undef Set_Flag
#undef Unset_Flag
#undef Flag_Setted
#undef Flag_Unsetted
#undef GetPageFlags

#endif // __PAGE_TABLE__
