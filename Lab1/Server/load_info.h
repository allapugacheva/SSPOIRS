#ifndef LOAD_INFO_H
#define LOAD_INFO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 80

typedef struct {
    char client[INET_ADDRSTRLEN];
    char fileName[BUFFER_SIZE];
    int fileSize;
    int processed;
    int download;
} LOAD_INFO;

void init_load_info_client(LOAD_INFO* cur, const char* client);
void init_load_info_file(LOAD_INFO* cur, const char* file, FILE* f, int download);
int same_clients_files(LOAD_INFO* cur, LOAD_INFO* last);
void copy_file(LOAD_INFO* dst, LOAD_INFO* src, FILE* f);
void copy_info(LOAD_INFO* dst, LOAD_INFO* src);

#endif