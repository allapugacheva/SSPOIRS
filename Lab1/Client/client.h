#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>
#include "../bnl/check_connection.h"
#include "../bnl/settings.h"
#include "../bnl/loading_line.h"

#ifdef _WIN32
#include <Ws2def.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <windows.h>
#include <conio.h>
//#pragma comment(lib, "ws2_32.lib")
#elif __linux__
#include <sys/socket.h>
#include <netdb.h>
#endif

#define BUFFER_SIZE 2048

void run         (const char* server, int port);
int  start_client(SCKT* cfd, const char* serverName, int port);
int  upload      (SCKT* cfd, const wchar_t* file);
int  download    (SCKT* cfd, const wchar_t* file);

#endif
