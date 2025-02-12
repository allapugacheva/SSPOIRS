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
#include "logger.h"

void setup_keepalive(int* sockfd);
int check_connection(int* sockfd, FILE* logger);
int write_with_check(int* sockfd, const char* buffer, int len, FILE* logger, FILE* f);
int read_with_check(int* sockfd, char** buffer, int len, FILE* logger, FILE* f);
int write_with_check_int(int* sockfd, long double* buffer, FILE* logger, FILE* f);
int read_with_check_int(int* sockfd, long double* buffer, FILE* logger, FILE* f);

#endif