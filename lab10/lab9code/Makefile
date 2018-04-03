all: kv-server
.PHONY: clean all

CC=gcc
CFLAGS=-Werror -Wall -pedantic

kv-server: kv-server.c kv-server.h
	$(CC) $(CFLAGS) -o kv-server kv-server.c

clean:
	rm -f kv-server
