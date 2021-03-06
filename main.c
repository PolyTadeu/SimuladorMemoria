#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "premissas.h"
#include "pagetable.c"

typedef struct _GlobalData {
    u32 num_procs;
    PageTable *vtables;     // size is num_procs
    u8 *mem;                // size is MEM_SIZE
    u8 *disk;               // size is num_procs*MAX_PAGE*FRAME_SIZE
    LRUg *lrug;
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

    global.lrug = malloc(sizeof(*global.lrug));
    if ( !global.lrug ) {
        printf("Sem memoria para alocar lru grande (%lu bytes)\n",
                sizeof(*global.lrug));
        exit(1);
    }
    init_lrug(global.lrug, MAX_FRAME);

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
    deinit_lrug(global.lrug);
    free(global.lrug);
    free(global.vtables);
    free(global.mem);
}

void* getVoidVtable(Pid pid) {
    assert( pid < global.num_procs );
    return global.vtables + pid;
}

void copy_from_disk(Pid pid, PageNum page, FrameIdx frame) {
    fprintf(OUT, "Copiando do disco: "
            "pid: "S_PID", page: %hhu, frame: %hhu\n",
            P_PID(pid), page, frame);
    const u64 diskIdx = (pid * MAX_PAGE + page) * FRAME_SIZE;
    const u64 memIdx = frame * FRAME_SIZE;
    for ( u64 i = 0; i < FRAME_SIZE; i++ ) {
        global.mem[memIdx + i] = global.disk[diskIdx + i];
    }
}

void copy_to_disk(Pid pid, PageNum page, FrameIdx frame) {
    fprintf(OUT, "Copiando para o disco: "
            "pid: "S_PID", page: %hhu, frame: %hhu\n",
            P_PID(pid), page, frame);
    const u64 diskIdx = (pid * MAX_PAGE + page) * FRAME_SIZE;
    const u64 memIdx = frame * FRAME_SIZE;
    for ( u64 i = 0; i < FRAME_SIZE; i++ ) {
        global.disk[diskIdx + i] = global.mem[memIdx + i];
    }
}

void trace(FILE *f, Pid pid) {
    fprintf(f, "\n>>>>>\n");
    traceTable(f, global.vtables + pid);
    traceFrames(f, global.lrug);
    fprintf(f, "<<<<<\n\n");
}

u8 read_addr(Pid pid, Vaddr addr) {
    pthread_mutex_lock(global.lock);

    printf("Processo "S_PID": read_addr "S_VADDR"\n",
            P_PID(pid), P_VADDR(addr));
    PageTable *vtable = global.vtables + pid;
    if ( !isLoaded(vtable, addr) ) {
        printf("=== PageFault ===\n");
        loadPage(global.lrug, vtable, pid, addr);
    }
    markPageUsed(global.lrug, vtable, pid, addr);
    FrameIdx frame = getFrameIdx(vtable, addr);
    Faddr real = RealAddr(frame, addr);

    trace(OUT, pid);

    pthread_mutex_unlock(global.lock);
    return global.mem[real];
}

void write_addr(Pid pid, Vaddr addr, u8 byte) {
    pthread_mutex_lock(global.lock);

    printf("Processo "S_PID": write_addr "S_VADDR" (%hhu)\n",
            P_PID(pid), P_VADDR(addr), byte);
    PageTable *vtable = global.vtables + pid;
    if ( !isLoaded(vtable, addr) ) {
        printf("=== PageFault ===\n");
        loadPage(global.lrug, vtable, pid, addr);
    }
    markPageModified(global.lrug, vtable, pid, addr);
    FrameIdx frame = getFrameIdx(vtable, addr);
    Faddr real = RealAddr(frame, addr);
    global.mem[real] = byte;

    trace(OUT, pid);

    pthread_mutex_unlock(global.lock);
}

typedef struct _ProcIn {
    Pid pid;
} ProcIn;

void* process(void *v) {
    ProcIn pin = *(ProcIn *) v;
    Pid pid = pin.pid;
    while ( 1 ) {
        if ( rand() & 1 ) {
            u8 byte = read_addr(pid, rand() % MAX_ADDR);
            printf("Processo "S_PID": read 0x%02hhx\n",
                   P_PID(pid), byte);
        } else {
            Vaddr addr = rand() % MAX_ADDR;
            write_addr(pid, addr, pid * MAX_ADDR + addr);
        }
        sleep(SLEEP_TIME);
    }
    printf("Saindo "S_PID"!\n", pid);
    return NULL;
}

int main() {
    const u32 lenpids = MAX_THREADS;

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
        printf("Criando Processo %u.\n", i);
        pthread_create(pids + i, NULL, process, pins + i);
        sleep(SLEEP_TIME);
    }

    for ( u32 i = 0; i < lenpids; i++ ) {
        pthread_join(pids[i], NULL);
        printf("Main: join processo %u.\n", i);
    }

    free(pins);
    free(pids);
    destroy_global();

    return 0;
}
