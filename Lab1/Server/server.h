#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include "load_info.h"
#include "../bnl/check_connection.h"
#include "../bnl/settings.h"
#include "../bnl/loading_line.h"

void run           (int port);
int  start_server  (int* sfd, int port);
int  process_client(int* cfd);
int  receive_data  (int* cfd, const char* file);
int  send_data     (int* cfd, const char* file);
void echo          ();
void server_time   ();

#endif
