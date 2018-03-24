
.PHONY: test clean

# Can't have unused variables 
# Can't have unused parameters 
# No newline at end of file 
CC=gcc
CFLAGS= -g -Wall -pedantic -Werror -Wextra 


OBJS=part1.o

all: part1.o

part1.o: part1.c 
		$(CC) $(CFLAGS) -c part1.c

part1: $(OBJS)
		$(CC) $(CFLAGS) -I./ part1.c -o part1.o 


clean:
	rm -f part1.o

