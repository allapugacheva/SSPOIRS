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
#include "../bnl/check_connection.h"
#include "../bnl/settings.h"
#include "../bnl/loading_line.h"

#define BUFFER_SIZE 2048

void run         (const char* server, int port);
int  start_client(int* cfd, const char* serverName, int port);
int  upload      (int* cfd, const char* file);
int  download    (int* cfd, const char* file);

#endif
