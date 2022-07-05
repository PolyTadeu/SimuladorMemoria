#ifndef __LRU_g__
#define __LRU_g__

#include "types.h"
#include "premissas.h"

// Nota: é circular
typedef struct _LRUg_Node {
    Pid pid;
    PageNum page;
    FrameIdx next;
    FrameIdx prev;
} LRUg_Node;

typedef struct _LRUg {
    // O nodes[MAX_FRAME] é
    // a cabeça da lista de frames ocupados
    LRUg_Node nodes[MAX_FRAME+1];
    FrameIdx free;
} LRUg;

void init_lrug(LRUg *lru) {
    LRUg l = {
        .free = 0,
    };
    for ( u64 i = 0; i < MAX_FRAME; i++ ) {
        l.nodes[i].next = i+1;
        l.nodes[i].prev = i-1;
    }
    l.nodes[0].prev = MAX_FRAME;
    l.nodes[MAX_FRAME].next = MAX_FRAME;
    l.nodes[MAX_FRAME].prev = MAX_FRAME;
    *lru = l;
}

b32 is_full_g(LRUg *lru) {
    return lru->free == MAX_FRAME;
}

void remove_node(LRUg *lru, FrameIdx frame) {
    const FrameIdx next = lru->nodes[frame].next;
    const FrameIdx prev = lru->nodes[frame].prev;
    lru->nodes[prev].next = next;
    lru->nodes[next].prev = prev;
}

void put_node_before(LRUg *lru, FrameIdx frame,
        FrameIdx before) {
    const FrameIdx last = lru->nodes[before].prev;
    lru->nodes[frame].next = before;
    lru->nodes[frame].prev = last;
    lru->nodes[last].next = frame;
    lru->nodes[before].prev = frame;
}

FrameIdx alloc_page_g(LRUg *lru) {
    assert( !is_full_g(lru) );
    FrameIdx frame = lru->free;
    {
        // Tira da lista de free
        remove_node(lru, frame);
        lru->free = lru->nodes[frame].next;
    }
    {
        // Coloca no final da lista
        put_node_before(lru, frame, MAX_FRAME);
    }
    return frame;
}

void markUsed_g(LRUg *lru, FrameIdx frame) {
    for ( FrameIdx now = lru->nodes[frame].next;
            now != MAX_FRAME;
            now = lru->nodes[now].next ) {
        assert( now != frame && "Marking free frame as used" );
    }
    remove_node(lru, frame);
    put_node_before(lru, frame, MAX_FRAME);
}

FrameIdx dequeue_g(LRUg *lru) {
    const FrameIdx frame = lru->nodes[MAX_FRAME].next;
    assert( frame < MAX_FRAME );
    markUsed_g(lru, frame);
    return frame;
}

#endif // __LRU_g__
