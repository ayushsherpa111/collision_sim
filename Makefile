CC=clang
CFLAGS=-Wall -lSDL2_gpu -lm -lSDL2 -g -I.
OUT=sdl_gpu
DEPS=./src/cust_type/vector.h
SRC=$(shell find src -name '*.c')
OBJ=${SRC:.c=.o}

all: build clean

build: ${OBJ}
	${CC} $^ -o ${OUT} ${CFLAGS}

%.o: %.c ${DEPS}
	${CC} -g -c -o $@ $< 

clean:
	rm ${OBJ}
