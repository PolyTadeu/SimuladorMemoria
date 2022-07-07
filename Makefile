CC = gcc
CFLAGS = -Wall -Wextra -Werror

all: run_exe

run_exe: run
	./run

run: main.c types.h premissas.h pagetable.c lru_pequeno.c lru_grande.c
	${CC} ${CFLAGS} main.c -o $@

test_g: test_g.out
	./test_g.out

test_g.out: test/grande.c lru_grande.c
	${CC} ${CFLAGS} test/grande.c -o $@

clean:
	rm -f *.out run
