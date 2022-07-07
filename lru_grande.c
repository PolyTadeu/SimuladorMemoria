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
    // O nodes[frame_cnt] é
    // a cabeça da lista de frames ocupados
    // tem tamanho frame_cnt+1
    LRUg_Node *nodes;
    FrameIdx free;
    u32 frame_cnt;
} LRUg;

void init_lrug(LRUg *lru, u32 frame_cnt) {
    LRUg_Node *nodes = malloc(sizeof(*nodes)*(frame_cnt+1));
    if ( !nodes ) {
        printf("Sem memoria para alocar lista de nodes do "
               "lru grande (%lu bytes)\n",
                sizeof(*nodes)*frame_cnt);
        exit(1);
    }
    for ( u64 i = 0; i < frame_cnt; i++ ) {
        nodes[i].next = i+1;
        nodes[i].prev = i-1;
    }
    nodes[0].prev = frame_cnt;
    nodes[frame_cnt].next = frame_cnt;
    nodes[frame_cnt].prev = frame_cnt;

    LRUg l = {
        .nodes = nodes,
        .free = 0,
        .frame_cnt = frame_cnt,
    };
    *lru = l;
}

void deinit_lrug(LRUg *lru) {
    free(lru->nodes);
}

b32 is_full_g(LRUg *lru) {
    return lru->free == lru->frame_cnt;
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
        const FrameIdx next = lru->nodes[frame].next;
        if ( frame == next ) {
            lru->free = lru->frame_cnt;
        } else {
            lru->free = next;
        }
    }
    {
        // Coloca no final da lista
        put_node_before(lru, frame, lru->frame_cnt);
    }
    return frame;
}

void markUsed_g(LRUg *lru, FrameIdx frame) {
    for ( FrameIdx now = lru->nodes[frame].next;
            now != lru->frame_cnt;
            now = lru->nodes[now].next ) {
        assert( now != frame && "Marking free frame as used" );
    }
    remove_node(lru, frame);
    put_node_before(lru, frame, lru->frame_cnt);
}

FrameIdx dequeue_g(LRUg *lru) {
    const FrameIdx frame = lru->nodes[lru->frame_cnt].next;
    assert( frame < lru->frame_cnt );
    markUsed_g(lru, frame);
    return frame;
}

#endif // __LRU_g__
