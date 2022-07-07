#ifndef __LRU_p__
#define __LRU_p__

#include "types.h"
#include "premissas.h"

typedef struct _LRUp {
    PageNum queue[WORKING_SET];
    // Proxima pagina a ser tirada
    u8 next;
    u8 size;
} LRUp;

b32 is_full_p(LRUp *lru) {
    return lru->size >= WORKING_SET;
}

void alloc_page_p(LRUp *lru, PageNum page) {
    assert( !is_full_p(lru) );
    u8 idx = (lru->next + lru->size) % WORKING_SET;
    lru->queue[idx] = page;
    lru->size += 1;
}

void markUsed_p(LRUp *lru, PageNum page) {
    b32 found = 0;
    for ( u8 i = 0; i < lru->size; i++ ) {
        const u8 this = (lru->next + i) % WORKING_SET;
        const u8 prev = (lru->next + i - 1) % WORKING_SET;
        if ( found ) {
            assert( i > 0 );
            const PageNum tmp = lru->queue[prev];
            lru->queue[prev] = lru->queue[this];
            lru->queue[this] = tmp;
        } else {
            if ( lru->queue[this] == page ) {
                found += 1;
            }
        }
    }
    assert( found > 0 );
    assert( found == 1 && "unreachable" );
}

PageNum dequeue_p(LRUp *lru) {
    assert( lru->size > 0 );
    const PageNum page = lru->queue[lru->next];
    lru->next = (lru->next + 1) % WORKING_SET;
    lru->size -= 1;
    return page;
}

void trace_LRUp(FILE* f, const LRUp *lru) {
    fprintf(f, "LRUp:");
    for ( u8 i = 0; i < lru->size; i++ ) {
        const u8 this = ( lru->next + i ) % WORKING_SET;
        const PageNum page = lru->queue[this];
        fprintf(f, " %2hhu", page);
    }
    fprintf(f, "\n");
}

#endif // __LRU_p__
