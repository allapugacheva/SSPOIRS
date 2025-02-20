#ifndef CHECK_CONNECTION_H
#define CHECL_CONNECTION_H

#include <stdio.h>     
#include <stdlib.h>    
#include <unistd.h>   
#include <errno.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

void setup_keepalive      (int* sockfd);
int  check_connection     (int* sockfd);
int  write_with_check_str (int* sockfd, const char* buffer, int len, FILE* f);
int  read_with_check_str  (int* sockfd, char** buffer, int len, FILE* f);
int  write_with_check_long(int* sockfd, long* buffer, FILE* f);
int  read_with_check_long (int* sockfd, long* buffer, FILE* f);
int  process_result       (int ret, int* sockfd, FILE* f);

#endif