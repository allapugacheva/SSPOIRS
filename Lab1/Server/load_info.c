#include "load_info.h"

void init_load_info_client(LOAD_INFO* cur, const wchar_t* client) {

    wcscpy(cur->client, client);
    cur->fileName[0] = L'\0';
    cur->fileSize = -1;
    cur->processed = 0;
}
void init_load_info_file(LOAD_INFO* cur, const wchar_t* file, FILE* f, int download) {

    cur->download = download;
    wcscpy(cur->fileName, file);

    if (f != NULL) {
        fseek(f, 0, SEEK_END);
        cur->fileSize = ftell(f);
        rewind(f);
    }
}
int same_clients_files(LOAD_INFO* cur, LOAD_INFO* last) {

    return wcscmp(cur->client, last->client) == 0 && wcscmp(cur->fileName, last->fileName) == 0 && cur->download == last->download;
}
void copy_file(LOAD_INFO* dst, LOAD_INFO* src, FILE* f) {

    dst->processed = src->processed;

    fseek(f, dst->processed, SEEK_SET);
}

void copy_info(LOAD_INFO* dst, LOAD_INFO* src) {

    wcscpy(dst->client, src->client);
    wcscpy(dst->fileName, src->fileName);
    dst->fileSize = src->fileSize;
    if (src->fileSize <= src->processed)
        dst->processed = 0;
    else
        dst->processed = src->processed;
    src->fileSize = -1;
    src->processed = 0;
    dst->download = src->download;
}