CC = gcc
CFLAGS = -Wall -Wextra -pedantic -Werror -ansi -O3 -g
OBJS = $(addsuffix .o, $(basename $(wildcard src/*.c)))
PROG_NAMES = cat head
PROGS = $(addprefix bin/, $(PROG_NAMES))

all: $(OBJS) $(PROG_NAMES)

%: src/%.o
	$(CC) -o bin/$@ $^

clean:
	$(RM) $(OBJS) $(PROGS)
