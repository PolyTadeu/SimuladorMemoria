#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "premissas.h"
#include "lru_pequeno.c"
#include "pagetable.c"

typedef struct _GlobalData {
    u32 num_procs;
    PageTable *vtables;     // size is num_procs
    u8 *mem;                // size is MEM_SIZE
    u8 *disk;               // size is num_procs*MAX_PAGE*FRAME_SIZE
    pthread_mutex_t *lock;
} GlobalData;

GlobalData global;

void create_global(const u32 num_procs) {
    global.num_procs = num_procs;

    global.vtables = malloc(sizeof(*global.vtables)*num_procs);
    if ( !global.vtables ) {
        printf("Sem memoria para alocar %u PageTables (%lu bytes)\n",
                num_procs, sizeof(*global.vtables)*num_procs);
        exit(1);
    }

    global.mem = malloc(sizeof(*global.mem)*MEM_SIZE);
    if ( !global.mem ) {
        printf("Sem memoria para alocar memória (%lu bytes)\n",
                sizeof(*global.mem)*MEM_SIZE);
        exit(1);
    }

    global.disk = malloc(
            sizeof(*global.disk)*num_procs*MAX_PAGE*FRAME_SIZE);
    if ( !global.disk ) {
        printf("Sem memoria para alocar disk (%lu bytes)\n",
                sizeof(*global.disk)*num_procs*MAX_PAGE*FRAME_SIZE);
        exit(1);
    }

    global.lock = malloc(sizeof(*global.lock));
    if ( !global.lock ) {
        printf("Sem memoria para alocar global.lock (%lu bytes)\n",
                sizeof(*global.lock));
        exit(1);
    }
    if ( pthread_mutex_init(global.lock, NULL) ) {
        printf("pthread_mutex_init falhou!\n");
        exit(1);
    }
}

void destroy_global() {
    pthread_mutex_destroy(global.lock);
    free(global.lock);
    free(global.vtables);
    free(global.mem);
}

u8 read_addr(Pid pid, Vaddr addr) {
    pthread_mutex_lock(global.lock);

    printf("Processo "S_PID": read_addr "S_VADDR"\n",
            P_PID(pid), P_VADDR(addr));
    PageTable *vtable = global.vtables + pid;
    if ( !isLoaded(vtable, addr) ) {
        loadPage(vtable, pid, addr);
    }
    markPageUsed(vtable, addr);
    FrameIdx frame = getFrameIdx(vtable, addr);
    Faddr real = RealAddr(frame, addr);

    pthread_mutex_unlock(global.lock);
    return global.mem[real];
}

void write_addr(Pid pid, Vaddr addr, u8 byte) {
    pthread_mutex_lock(global.lock);

    printf("Processo "S_PID": write_addr "S_VADDR" (%hhu)\n",
            P_PID(pid), P_VADDR(addr), byte);
    PageTable *vtable = global.vtables + pid;
    if ( !isLoaded(vtable, addr) ) {
        loadPage(vtable, pid, addr);
    }
    markPageUsed(vtable, addr);
    FrameIdx frame = getFrameIdx(vtable, addr);
    Faddr real = RealAddr(frame, addr);
    global.mem[real] = byte;

    pthread_mutex_unlock(global.lock);
}

typedef struct _ProcIn {
    Pid pid;
} ProcIn;

typedef struct _ProcOut {
} ProcOut;

void* func(void *v) {
    ProcIn pin = *(ProcIn *) v;
    Pid pid = pin.pid;
    ProcOut *ret = malloc(sizeof(*ret));
    if ( !ret ) {
        fprintf(stderr, "Processo "S_PID": sem memória para allocar"
                " ProcOut (%lu bytes)\n", P_PID(pid), sizeof(*ret));
        return NULL;
    }
    u8 max_page = rand() % MAX_PAGE;
    u8 i = max_page + 1;
    printf("Thread "S_PID"! max_page: %hhu\n", P_PID(pid), max_page);
    while ( i > 0 ) {
        printf("Processo "S_PID"!\n", P_PID(pid));
        if ( rand() & 1 ) {
            u8 byte = read_addr(pid, rand() % MAX_ADDR);
            printf("Processo "S_PID": read 0x%02hhx\n",
                   P_PID(pid), byte);
        } else {
            Vaddr addr = rand() % MAX_ADDR;
            write_addr(pid, addr, pid * MAX_ADDR + addr);
        }
        sleep(SLEEP_TIME);
        i -= 1;
    }
    printf("Saindo "S_PID"! ret: %p\n", pid, ret);
    return ret;
}

int main(const int argc, const char **argv) {
    u32 lenpids = 3;

    if ( argc > 1 ) {
        for ( int i = 1; i < argc; i++ ) {
            printf("%d: %s\n", i, argv[i]);
        }
    } else {
        printf("Sem argumentos!\n");
        printf("TODO: print help\n");
        exit(0);
    }

    create_global(lenpids);

    pthread_t *pids = malloc(sizeof(*pids)*lenpids);
    if ( !pids ) {
        printf("Sem memoria para alocar %d pids (%lu bytes)\n",
                lenpids, sizeof(*pids)*lenpids);
        exit(1);
    }

    ProcIn *pins = malloc(sizeof(*pins)*lenpids);
    if ( !pins ) {
        printf("Sem memoria para alocar %d pins (%lu bytes)\n",
                lenpids, sizeof(*pins)*lenpids);
        exit(1);
    }

    for ( u32 i = 0; i < lenpids; i++ ) {
        ProcIn pin = {
            .pid = i,
        };
        pins[i] = pin;
        pthread_create(pids + i, NULL, func, pins + i);
        sleep(SLEEP_TIME);
    }

    for ( u32 i = 0; i < lenpids; i++ ) {
        ProcOut *ret;
        pthread_join(pids[i], (void **) &ret);
        printf("Main: join processo %u. ret: %p\n", i, ret);
        free(ret);
    }

    free(pins);
    free(pids);
    destroy_global();

    return 0;
}