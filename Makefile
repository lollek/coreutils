CC = gcc
CFLAGS = -Wall -Wextra -pedantic -Werror -ansi -O3 -g
OBJS = $(addsuffix .o, $(basename $(wildcard src/*.c)))
PROG_NAMES = cat
PROGS = $(addprefix bin/, $(PROG_NAMES))

all: $(OBJS) $(PROG_NAMES)

cat: src/cat.o
	$(CC) -o bin/cat $^

clean:
	$(RM) $(OBJS) $(PROGS)
