CC = gcc
FLAGS = -pg

.PHONY: clean

all: server client
server: Server/server.c Server/main.c Server/server.h logger.h logger.c settings.c settings.h check_connection.c check_connection.h Server/load_info.h Server/load_info.c Makefile
		$(CC) $(FLAGS) Server/server.c Server/main.c logger.c check_connection.c settings.c Server/load_info.c -o Server/server
client: Client/client.c Client/main.c Client/client.h logger.h logger.c settings.c settings.h check_connection.c check_connection.h Makefile
		$(CC) $(FLAGS) Client/client.c Client/main.c logger.c check_connection.c settings.c -o Client/client
clean:
		rm server client