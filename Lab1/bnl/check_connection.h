#ifndef CHECK_CONNECTION_H
#define CHECL_CONNECTION_H

#include <stdio.h>     
#include <stdlib.h>    
#include <errno.h>
#include <wchar.h>
#include <unistd.h>   
#include <sys/types.h> 

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2def.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#define SCKT SOCKET
#define INVLD_SCKT INVALID_SOCKET
#elif __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#define SCKT int
#define INVLD_SCKT -1
#endif

void setup_keepalive      (SCKT* sockfd);
int  check_connection     (SCKT* sockfd);
int  write_with_check_str (SCKT* sockfd, const char* buffer, int len, FILE* f);
int  read_with_check_str  (SCKT* sockfd, char** buffer, int len, FILE* f);
int  write_with_check_wstr (SCKT* sockfd, const wchar_t* buffer, int len, FILE* f);
int  read_with_check_wstr  (SCKT* sockfd, wchar_t** buffer, int len, FILE* f);
int  write_with_check_long(SCKT* sockfd, long* buffer, FILE* f);
int  read_with_check_long (SCKT* sockfd, long* buffer, FILE* f);
int  process_result       (int ret, SCKT* sockfd, FILE* f);

#endif