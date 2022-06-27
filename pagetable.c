#ifndef __PAGE_TABLE__
#define __PAGE_TABLE__

#include "premissas.h"
#include "lru_pequeno.c"

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
    LRUp lru;
} PageTable;

b32 isLoaded(PageTable *vtable, Vaddr addr) {
    return Is_Flag_Set(PageFlag_Loaded,
            GetPageFlags(vtable, addr));
}

void copy_from_disk(Pid pid, PageNum page, FrameIdx frame) {
    (void) pid; (void) page; (void) frame;
    assert(0 && "copy_from_disk is not implemented");
}

void unloadPage(PageTable *vtable, Pid pid, PageNum page) {
    (void) vtable; (void) pid; (void) page;
    assert(0 && "unloadPage is not implemented");
}

void loadPage(PageTable *vtable, Pid pid, Vaddr addr) {
    (void) vtable; (void) addr;
    LRUp *lru = &(vtable->lru);
    if ( is_full_p(lru) ) {
        const PageNum page = dequeue_p(lru);
        const FrameIdx frame = vtable->page[page].frame;
        unloadPage(vtable, pid, page);
        copy_from_disk(pid, VirtPage(addr), frame);
    } else {
        assert(0 && "Precisa do LRU Grande");
    }
}

void markPageUsed(PageTable *vtable, Vaddr addr) {
    assert(isLoaded(vtable, addr)
            && "Trying to makePageUsed a unloaded Page");
    markUsed_p(&(vtable->lru), VirtPage(addr));
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
