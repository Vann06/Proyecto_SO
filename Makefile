CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -Iinclude

all: test_protocol

test_protocol: src/protocol.c src/utils.c tests/test_protocol.c
	$(CC) $(CFLAGS) -o test_protocol src/protocol.c src/utils.c tests/test_protocol.c

clean:
	rm -f test_protocol