#ifndef LOAD_INFO_H
#define LOAD_INFO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#elif __linux__
#include <arpa/inet.h>
#endif

#define BUFFER_SIZE 2048 

typedef struct {
    wchar_t client[INET_ADDRSTRLEN];
    wchar_t fileName[BUFFER_SIZE];
    long fileSize;
    long processed;
    int download;
} LOAD_INFO;

void init_load_info_client(LOAD_INFO* cur, const wchar_t* client);
void init_load_info_file  (LOAD_INFO* cur, const wchar_t* file, FILE* f, int download);
int  same_clients_files   (LOAD_INFO* cur, LOAD_INFO* last);
void copy_file            (LOAD_INFO* dst, LOAD_INFO* src, FILE* f);
void copy_info            (LOAD_INFO* dst, LOAD_INFO* src);

#endif