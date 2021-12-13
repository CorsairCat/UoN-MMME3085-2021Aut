INCL= include/
BIN= build/
CFLAGS := -Iinclude -O2 -g -DBIG_ENDIAN
SRCS := $(wildcard *.c) 
OBJS := $(patsubst %c,%c.o,$(SRCS))

all:${OBJS} linkage
%.c.o:%.c
	@gcc ${CFLAGS} -c $< -o build/$@
linkage:
	@gcc -o ../bin/main build/*.o
clean:
	@rm build/* 2> /dev/null || echo > /dev/nu