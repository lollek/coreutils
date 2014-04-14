CC = gcc
CFLAGS = -Wall -Wextra -pedantic -Werror -ansi -O3 -g -std=c11
OBJS = cat.o
PROGS = cat

all: $(OBJS) cat

cat: cat.o
	$(CC) -o cat cat.o

clean:
	$(RM) $(OBJS) $(PROGS)
