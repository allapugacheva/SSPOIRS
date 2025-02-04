CC = gcc

.PHONY: clean

all: server client
server: Server/server.c Server/main.c Server/server.h logger.h logger.c check_connection.c check_connection.h Makefile
		$(CC) Server/server.c Server/main.c logger.c check_connection.c -o Server/server
client: Client/client.c Client/main.c Client/client.h logger.h logger.c check_connection.c check_connection.h Makefile
		$(CC) Client/client.c Client/main.c logger.c check_connection.c -o Client/client
clean:
		rm server client