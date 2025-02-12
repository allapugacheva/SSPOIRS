#include "load_info.h"

void init_load_info_client(LOAD_INFO* cur, const char* client) {

    strcpy(cur->client, client);
    cur->fileName[0] = '\0';
    cur->fileSize = -1;
    cur->processed = 0;
}
void init_load_info_file(LOAD_INFO* cur, const char* file, FILE* f, int download) {

    cur->download = download;
    strcpy(cur->fileName, file);

    if(f != NULL) {
        fseek(f, 0, SEEK_END);
        cur->fileSize = ftell(f);
        rewind(f);
    }
}
int same_clients_files(LOAD_INFO* cur, LOAD_INFO* last) {

    return strcmp(cur->client, last->client) == 0 && strcmp(cur->fileName, last->fileName) == 0 && cur->download == last->download;
}
void copy_file(LOAD_INFO* dst, LOAD_INFO* src, FILE* f) {

    dst->processed = src->processed;

    fseek(f, dst->processed, SEEK_SET);
}

void copy_info(LOAD_INFO* dst, LOAD_INFO* src) {

    strcpy(dst->client, src->client);
    strcpy(dst->fileName, src->fileName);
    dst->fileSize = src->fileSize;
    if(src->fileSize <= src->processed)
        dst->processed = 0;
    else
        dst->processed = src->processed;
    src->fileSize = -1;
    src->processed = 0;
    dst->download = src->download;
}