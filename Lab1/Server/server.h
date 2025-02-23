#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include "load_info.h"
#include "../bnl/check_connection.h"
#include "../bnl/settings.h"
#include "../bnl/loading_line.h"

#ifdef _WIN32
#include <Ws2def.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <windows.h>
#include <conio.h>
#elif __linux__
#include <sys/socket.h>
#include <netdb.h>
#endif

void run           (int port);
int  start_server  (SCKT* sfd, int port);
int  process_client(SCKT* cfd);
int  receive_data  (SCKT* cfd, const wchar_t* file);
int  send_data     (SCKT* cfd, const wchar_t* file);
void echo          ();
void server_time   ();

#endif
