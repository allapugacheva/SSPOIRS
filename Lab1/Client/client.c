#include "client.h"

SETTINGS* settings;

void run(const char* server, int port) {

    settings = init_settings();

    #ifdef __linux__
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    #endif

    SCKT cfd;
    wchar_t command[BUFFER_SIZE];

    wprintf(L"> ");
    int work = 1;
    while(work) {

        if (!start_client(&cfd, server, port)) {
            wprintf(L"\rОшибка при запуске клиента.\n");
            exit(errno);
        }
        while (1) {

            if (!check_connection(&cfd))
                break;

            #ifdef _WIN32
            if (!_kbhit())
                continue;
            #endif
                
            if (fgetws(command, sizeof(command), stdin) != NULL) {
                command[wcslen(command) - 1] = L'\0';

                if (wcsstr(command, L"UPLOAD") != NULL) {
                    if (upload(&cfd, command) == 0)
                        wprintf(L"\rОшибка выполнения команды UPLOAD.\n");
                }
                else if (wcsstr(command, L"DOWNLOAD") != NULL) {
                    if (download(&cfd, command) == 0)
                        wprintf(L"\rОшибка выполнения команды DOWNLOAD.\n");
                }
                else if (wcsstr(command, L"SETTINGS") != NULL)
                    settings_command(settings, command); 
                else if (wcsstr(command, L"QUIT") != NULL) {
                    write_with_check_wstr(&cfd, command, wcslen(command) + 1, NULL);
                    work = 0;
                    break;
                }
                wprintf(L"> ");
            }
        }

        if (work) {
            wprintf(L"\rПотеряно соединение с сервером. Восстановить соединение? [y/n]: ");
            wchar_t ch;
            while((ch = getwchar()) != L'y' && ch != L'n');
            fflush(stdin);
            if (ch != L'y')
                break;

            #ifdef _WIN32
            u_long mode = 0;
            ioctlsocket(cfd, FIONBIO, &mode);
            #endif
        }
    }

    #ifdef __linux__
    fcntl(STDIN_FILENO, F_SETFL, flags);
    #endif
    close(cfd);
}

int start_client(SCKT* cfd, const char* serverName, int port) {

    *cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*cfd == INVLD_SCKT) {
        close(*cfd);
        return 0;
    }

    #ifdef __linux__
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;
    setsockopt(*cfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(*cfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    #endif

    #if __linux__
    int snd_buf_size = BUFFER_SIZE;
    setsockopt(*cfd, SOL_SOCKET, SO_SNDBUF, &snd_buf_size, sizeof(snd_buf_size));
    setsockopt(*cfd, SOL_SOCKET, SO_RCVBUF, &snd_buf_size, sizeof(snd_buf_size));
    #endif

    setup_keepalive(cfd);

    struct hostent* server;
    struct sockaddr_in addr;
    server = gethostbyname(serverName);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    #ifdef _WIN32
    memcpy(&addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    #elif __linux__
    bcopy((char*)server->h_addr_list[0], (char*)&addr.sin_addr.s_addr, server->h_length);
    #endif

    int tries = 0, maxTries = 10;
    while (connect(*cfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        if (errno == ENOENT) {
            if (++tries >= maxTries) {
                close(*cfd);
                return 0;
            }
            sleep(1);
            continue;
        } else {
            close(*cfd);
            return 0;
        }
    }
    
    #ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(*cfd, FIONBIO, &mode);
    #endif

    return 1;
}

int upload(SCKT* cfd, const wchar_t* command) {

    long fileSize = -1, sent = 0, read;

    const wchar_t* filePath = command + 7;
    if (is_absolute_path(filePath) != 1)
        filePath = get_file_path(settings->file_path, filePath);

    size_t len = wcslen(filePath) * sizeof(wchar_t) + 1;
    char* utf8_filename = (char*)malloc(len);
    wcstombs(utf8_filename, filePath, len);

    FILE* f = fopen(utf8_filename, "rb");
    free(utf8_filename);
    if (f == NULL)
        return 0;

    if (write_with_check_wstr(cfd, command, wcslen(command) + 1, f) < 0)
        return 0;

    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    rewind(f);

    if (write_with_check_long(cfd, &fileSize, f) < 0)
        return 0;

    if (read_with_check_long(cfd, &sent, f) < 0)
        return 0;

    if (sent != 0)
        fseek(f, sent, SEEK_SET);

    double recTime = 0.0, packTime = 0.0;
    struct timeval start, end;
    LLINE* lline = init_lline(((double)sent / fileSize) * 100.0, fileSize);
    show_lline(lline);

    gettimeofday(&start, NULL);
    char* buffer = (char*)malloc(BUFFER_SIZE);
    while (sent < fileSize && (read = fread(buffer, 1, BUFFER_SIZE, f))) {
        if (write_with_check_str(cfd, buffer, read, f) < 0)
            return 0;
        sent += read;
        
        gettimeofday(&end, NULL);
        packTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6;
        if (refresh_lline(lline, ((double)sent / fileSize) * 100.0, packTime) == 1) {
            recTime += packTime;
            gettimeofday(&start, NULL);
        }
    }
    free_lline(lline);

    wprintf(L"\rФайл успешно загружен на сервер. Скорость: "GREEN"%ls"RESET"; Время: "CYAN"%.2f с."RESET"\n", 
                                    get_speed_stirng(get_speed(fileSize, 100.0, recTime)), recTime);
    fclose(f);
    free(buffer);
    return 1;
}

int download(SCKT* cfd, const wchar_t* command) {

    if (write_with_check_wstr(cfd, command, wcslen(command) + 1, NULL) < 0)
        return 0;

    long fileSize = -1, received = 0, len, rec;

    wchar_t* filePath = get_file_path(settings->file_path, get_filename(command + 9));

    len = wcslen(filePath) * sizeof(wchar_t) + 1;
    char* utf8_filename = (char*)malloc(len);
    wcstombs(utf8_filename, filePath, len);

    FILE* f = fopen(utf8_filename, "rb+");
    if (f == NULL)
        f = fopen(utf8_filename, "wb");
    free(utf8_filename);

    if (read_with_check_long(cfd, &fileSize, f) < 0)
        return 0;
    if (read_with_check_long(cfd, &received, f) < 0)
        return 0;
    if (received != 0)
        fseek(f, received, SEEK_SET);

    double recTime = 0.0, packTime = 0.0;
    struct timeval start, end;
    LLINE* lline = init_lline(((double)received / fileSize) * 100.0, fileSize);
    show_lline(lline);

    gettimeofday(&start, NULL);
    char* buffer = (char*)malloc(BUFFER_SIZE);
    while (received < fileSize) {

        rec = read_with_check_str(cfd, &buffer, BUFFER_SIZE, f);
        if (rec < 0)
            return 0;
        else if (rec == 0)
            continue;

        fwrite(buffer, sizeof(char), rec, f);
        received += rec;

        gettimeofday(&end, NULL);
        packTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6;
        if(refresh_lline(lline, ((double)received / fileSize) * 100.0, packTime) == 1) {
            recTime += packTime;
            gettimeofday(&start, NULL);
        }
    }
    free_lline(lline);
        
    wprintf(L"\rФайл успешно скачан с сервера. Скорость: "GREEN"%ls"RESET"; Время: "CYAN"%.2f с"RESET"\n", 
                                        get_speed_stirng(get_speed(fileSize, 100.0, recTime)), recTime);
    fclose(f);
    free(filePath);
    free(buffer);
    return 1;
}