#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../types.h"
#include "../lru_grande.c"

void trace(const LRUg *lru) {
    printf("\n");
    traceFrames(stdout, lru);
    printf("\n");
}

int main() {
    const u32 cnt = 6;
    LRUg lrug, *lru = &lrug;
    init_lrug(lru, cnt);

    trace(lru);

    for ( u8 i = 0; i < cnt; i++ ) {
        assert( !is_full_g(lru) );
        assert( i == alloc_page_g(lru) );
    }

    assert( is_full_g(lru) );

    for ( u8 i = 0; i < cnt*2; i++ ) {
        assert( i % cnt == dequeue_g(lru) );
        assert( is_full_g(lru) );
    }

    for ( i8 i = cnt-1; i >= 0; i-- ) {
        markUsed_g(lru, i);
        assert( is_full_g(lru) );
        for ( i8 j = 0; j < i; j++ ) {
            assert( j == dequeue_g(lru) );
            assert( is_full_g(lru) );
        }
        for ( i8 j = cnt-1; j >= i; j-- ) {
            assert( j == dequeue_g(lru) );
            assert( is_full_g(lru) );
        }
    }

    for ( i8 i = cnt-1; i >= 0; i-- ) {
        assert( i == dequeue_g(lru) );
        assert( is_full_g(lru) );
    }

    trace(lru);

    printf("Ok!\n");
    deinit_lrug(lru);
    return 0;
}
