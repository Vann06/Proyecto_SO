CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -Iinclude -pthread
LDFLAGS=-pthread

CLIENT_SOURCES=src/main_client.c src/client_receiver.c src/client_ui.c src/protocol.c src/utils.c
SERVER_SOURCES=src/main_server.c src/server.c src/server_handlers.c src/server_registry.c src/protocol.c src/utils.c

all: client server

client: $(CLIENT_SOURCES)
	$(CC) $(CFLAGS) -o client $(CLIENT_SOURCES) $(LDFLAGS)

server: $(SERVER_SOURCES)
	$(CC) $(CFLAGS) -o server $(SERVER_SOURCES) $(LDFLAGS)

clean:
	rm -f client server