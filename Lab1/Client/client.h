#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include "../logger.h"
#include "../check_connection.h"
#include "../settings.h"

#include "../bnl/loading_line.h"

#define LOG_FILE "client_log.txt"
#define BUFFER_SIZE 64000

void run(const char* server, int port);
int start_client(int* cfd, const char* serverName, int port);
int upload(int* cfd, const char* file);
int download(int* cfd, const char* file);
void settings_command(char* command);
#endif
