CC = gcc
CFLAGS = -Wall -Wextra -Werror

all: run_exe

run_exe: run
	./run 3

run: main.c types.h premissas.h pagetable.c
	${CC} ${CFLAGS} main.c -o $@

clean:
	rm -f *.out run
