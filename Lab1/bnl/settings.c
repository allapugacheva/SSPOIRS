#include "settings.h"

SETTINGS* init_settings() {

    SETTINGS* settings = (SETTINGS*)malloc(sizeof(SETTINGS));

    wcscpy(settings->file_path, L".");
    return settings;
}

int execute_settings(SETTINGS* settings, const wchar_t* command, const wchar_t* value) {

    int res = 1;
    if (wcscmp(command, SET_PATH) == 0)
        res = set_path(settings, value);
    else if (wcscmp(command, SETTINGS_LIST) == 0)
        wprintf(L"PATH: %ls\n", settings->file_path);
    
    return res;
}

int set_path(SETTINGS* settings, const wchar_t* dir) {

    int fd;
    if (!is_directory_exists(dir)) {

        char utf8_folder[MAX_PATH_SIZE];
        wcstombs(utf8_folder, dir, sizeof(utf8_folder));

        fd = mkdir(utf8_folder, 0755);
        wprintf(L"%d %s", fd, utf8_folder);
        if (fd == -1)
            return 0;
    }
    wcscpy(settings->file_path, dir);

    return 1;
}

int is_directory_exists(const wchar_t *path) {

    char utf8_path[MAX_PATH_SIZE];
    wcstombs(utf8_path, path, sizeof(utf8_path));

    struct stat info;
    if (stat(utf8_path, &info) != 0)
        return 0; 

    return (info.st_mode & __S_IFDIR) != 0; 
}

wchar_t *get_file_path(const wchar_t *folder, const wchar_t *filename) {

    if (folder == NULL || filename == NULL)
        return NULL;

    size_t folder_len = wcslen(folder);
    size_t filename_len = wcslen(filename);
    size_t separator_len = (folder_len > 0 && folder[folder_len - 1] != L'/');

    int n = folder_len + filename_len + separator_len + 1;
    wchar_t *full_path = (wchar_t*)malloc(n * sizeof(wchar_t));

    if (separator_len)
        swprintf(full_path, n, L"%ls/%ls", folder, filename);
    else
        swprintf(full_path, n, L"%ls%ls", folder, filename);

    return full_path;
}

int is_absolute_path(const wchar_t* path) {

    if (path == NULL)
        return -1;

    return path[0] == L'/';
}

wchar_t* get_filename(const wchar_t *path) {
    
    if (path == NULL)
        return NULL;
    
    const wchar_t *filename = wcsrchr(path, L'/');
    
    if (filename != NULL)
        return wcsdup(filename + 1); 
    else
        return wcsdup(path); 
}

void settings_command(SETTINGS* settings, wchar_t* command) {

    if (wcsstr(command, L".path") != NULL) {
        wchar_t* start_i = wcschr(command, L' ');
        if (start_i == NULL) 
            return;

        int last_i = wcslen(command) - 1;

        while (*(++start_i) == ' ');
 
        wchar_t dir[MAX_PATH_SIZE];
        if (*start_i == L'"' && command[last_i] == L'"') {
            start_i++; last_i--; 
            wcsncpy(dir, start_i, wcslen(start_i));
            dir[wcslen(start_i) - 1] = L'\0';
        } else 
            wcscpy(dir, start_i);
        
        execute_settings(settings, SET_PATH, dir);

    } else if (wcscmp(command, L"SETTINGS") == 0)
        execute_settings(settings, SETTINGS_LIST, NULL);
}