CC = gcc
FLAGS = -Wno-discarded-qualifiers

.PHONY: clean

all: server client
server: logger.h logger.c settings.c settings.h  Server/server.c Server/main.c Server/server.h check_connection.c check_connection.h Server/load_info.h Server/load_info.c  bnl/loading_line.c bnl/loading_line.h bnl/color.h  Makefile
		$(CC) $(FLAGS) Server/server.c Server/main.c logger.c check_connection.c settings.c Server/load_info.c bnl/loading_line.c -o Server/server
client: Client/client.c Client/main.c Client/client.h logger.h logger.c settings.c settings.h check_connection.c check_connection.h bnl/loading_line.c bnl/loading_line.h bnl/color.h  Makefile
		$(CC) Client/client.c Client/main.c logger.c check_connection.c settings.c  bnl/loading_line.c -o Client/client
clean:
		rm server client
