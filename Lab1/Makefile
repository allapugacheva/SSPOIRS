CC = gcc

.PHONY: clean

all: server client
server: Server/server.c Server/main.c Server/server.h bnl/settings.c bnl/settings.h bnl/check_connection.c bnl/check_connection.h Server/load_info.h Server/load_info.c bnl/color.h bnl/loading_line.c bnl/loading_line.h Makefile
		$(CC) Server/server.c Server/main.c bnl/check_connection.c bnl/settings.c Server/load_info.c bnl/loading_line.c -o Server/server
client: Client/client.c Client/main.c Client/client.h bnl/settings.c bnl/settings.h bnl/check_connection.c bnl/check_connection.h bnl/color.h bnl/loading_line.c bnl/loading_line.h Makefile
		$(CC) Client/client.c Client/main.c bnl/check_connection.c bnl/settings.c bnl/loading_line.c -o Client/client
clean:
		rm server client
