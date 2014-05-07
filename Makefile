CC = gcc
CFLAGS = -Wall -Wextra -pedantic -Werror -ansi -O3 -g -std=c99
OBJS = $(addsuffix .o, $(basename $(wildcard src/*.c)))
PROG_NAMES = cat ls
PROGS = $(addprefix bin/, $(PROG_NAMES))

all: $(OBJS) $(PROG_NAMES)

%: src/%.o
	$(CC) -o bin/$@ src/$@*.o

clean:
	$(RM) $(OBJS) $(PROGS)
