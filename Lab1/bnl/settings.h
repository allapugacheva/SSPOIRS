#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <wchar.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PATH_SIZE       1024
#define MAX_FILE_NAME_SIZE  255

#define SET_PATH        L"PATH"
#define SETTINGS_LIST   L"LIST"

typedef struct _SETTINGS {
    wchar_t file_path[MAX_PATH_SIZE];
} SETTINGS;

SETTINGS* init_settings      ();
int       execute_settings   (SETTINGS* settings, const wchar_t* command, const wchar_t* value);
int       set_path           (SETTINGS* settings, const wchar_t* dir);
int       is_directory_exists(const wchar_t *path);
wchar_t*  get_file_path      (const wchar_t *folder, const wchar_t *filename);
int       is_absolute_path   (const wchar_t* path);
wchar_t*  get_filename       (const wchar_t *path);
void      settings_command   (SETTINGS* settings, wchar_t* command);

#endif