CC = gcc
CFLAGS = -Wall -Wextra -Werror

all: run_exe

run_exe: run
	./run 3

run: main.c types.h premissas.h pagetable.c lru_pequeno.c lru_grande.c
	${CC} ${CFLAGS} main.c -o $@

test_g_exe: test_g
	./test_g.out

test_g: test/grande.c lru_grande.c
	${CC} ${CFLAGS} test/grande.c -o $@.out

clean:
	rm -f *.out run
