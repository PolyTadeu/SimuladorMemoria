#ifndef __PAGE_TABLE__
#define __PAGE_TABLE__

#include "premissas.h"
#include "lru_pequeno.c"
#include "lru_grande.c"

#define Is_Flag_Set(flag, thing)    (((thing) & (flag)) == (flag))
#define Set_Flag(flag, thing)       ((thing) |= (flag))
#define Unset_Flag(flag, thing)     (((thing) &= ~(flag)))
#define Flag_Setted(flag, thing)    ((thing) | (flag))
#define Flag_Unsetted(flag, thing)  ((thing) & (~(flag)))
#define GetPageFlags(vt, addr)      (vt)->page[VirtPage(addr)].flags

#define GetVtable(pid)              ((PageTable *)(getVoidVtable(pid)))

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

void unloadPage(PageTable *vtable, Pid pid, PageNum page) {
    PageLine *pline = vtable->page + page;

    printf("---------\n");
    printf("pid: "S_PID", page: %hhu\n",
            P_PID(pid), page);
    printf("pline->flags: %X, pline->frame: %hhu\n",
            pline->flags, pline->frame);
    printf("---------\n");
    assert( Is_Flag_Set(PageFlag_Loaded, pline->flags) );

    if ( Is_Flag_Set(PageFlag_Modified, pline->flags) ) {
        copy_to_disk(pid, page, pline->frame);
    }

    pline->flags = 0;
}

void loadPage(LRUg *lrug, PageTable *vtable, Pid pid, Vaddr addr) {
    LRUp *lru = &(vtable->lru);
    FrameIdx frame;
    if ( is_full_p(lru) ) {
        const PageNum page = dequeue_p(lru);
        frame = vtable->page[page].frame;
        unloadPage(vtable, pid, page);
    } else {
        if ( is_full_g(lrug) ) {
            frame = dequeue_g(lrug);
            const LRUg_Node node = lrug->nodes[frame];
            unloadPage(GetVtable(node.pid), node.pid, node.page);
        } else {
            frame = alloc_page_g(lrug);
        }
    }
    alloc_page_p(lru, VirtPage(addr));
    vtable->page[VirtPage(addr)].frame = frame;
    vtable->page[VirtPage(addr)].flags = PageFlag_Loaded;
    copy_from_disk(pid, VirtPage(addr), frame);
    Set_Flag(PageFlag_Loaded, GetPageFlags(vtable, addr));
}

FrameIdx getFrameIdx(PageTable *vtable, Vaddr addr) {
    assert(isLoaded(vtable, addr)
            && "Trying to getFrameIdx from a unloaded Page");
    return vtable->page[VirtPage(addr)].frame;
}

void markPageUsed(LRUg *lrug, PageTable *vtable, Vaddr addr) {
    assert(isLoaded(vtable, addr)
            && "Trying to makePageUsed a unloaded Page");
    markUsed_p(&(vtable->lru), VirtPage(addr));
    markUsed_g(lrug, getFrameIdx(vtable, addr));
}

void markPageModified(LRUg *lrug, PageTable *vtable, Vaddr addr) {
    markPageUsed(lrug, vtable, addr);
    Set_Flag(PageFlag_Modified, GetPageFlags(vtable, addr));
}

void traceTable(FILE *f, const PageTable *vtable) {
    fprintf(f, " Page |  Flags | Frame\n");
    for ( u32 i = 0; i < MAX_PAGE; i++ ) {
        const PageLine pl = vtable->page[i];
        if ( Is_Flag_Set(PageFlag_Loaded, pl.flags) ) {
            fprintf(f, " %4u |  0x%02hhx  |  %02hhu\n",
                    i, pl.flags, pl.frame);
        }
    }
    trace_LRUp(f, &(vtable->lru));

}

#undef Is_Flag_Set
#undef Set_Flag
#undef Unset_Flag
#undef Flag_Setted
#undef Flag_Unsetted
#undef GetPageFlags

#undef GetVtable

#endif // __PAGE_TABLE__
