#include "server.h"

SETTINGS* settings;
LOAD_INFO current, last;

void run(int port) {

    settings = init_settings();

    #if __linux__
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    #endif

    wchar_t command[BUFFER_SIZE];
    SCKT sfd, cfd = INVLD_SCKT;
    if (!start_server(&sfd, port)) {
        wprintf(L"Ошибка при запуске сервера.\n");
        exit(errno);
    }
    
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    wprintf(L"> ");
    while (1) {

        if (cfd == INVLD_SCKT) {

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sfd, &readfds);
        
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;
        
            int ready = select(sfd + 1, &readfds, NULL, NULL, &timeout);
        
            if (ready == SO_ERROR) {
                wprintf(L"SELECT ERROR\n");
                return;
            }
        
            if (ready != 0 && FD_ISSET(sfd, &readfds)) {
                if ((cfd = accept(sfd, (struct sockaddr*)&clientAddr, &clientLen)) == INVLD_SCKT) {

                    #ifdef _WIN32
                    if (WSAGetLastError() != WSAEWOULDBLOCK)
                    #elif __linux__
                    if (errno != EWOULDBLOCK && errno != EAGAIN)
                    #endif
                    {
                        wprintf(L"\rОшибка соединения с клиентом.\n");
                        close(sfd);
                        close(cfd);
                        exit(errno);
                    }
                } else {
                    char clientIp[BUFFER_SIZE];
                    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, BUFFER_SIZE);

                    wchar_t unicode_clientIp[BUFFER_SIZE];
                    mbstowcs(unicode_clientIp, clientIp, BUFFER_SIZE);

                    init_load_info_client(&current, unicode_clientIp);
                    wprintf(L"\rПринято соединение с клиентом: "CYAN"%ls"RESET"\n> ", unicode_clientIp);
                }
            }
        } else {      
            if (check_connection(&cfd) == 0)
                wprintf(L"\rПотеряно соединение с клиентом: "CYAN"%ls"RESET"\n> ", current.client);
            else {
                if (process_client(&cfd)) {
                    close(cfd);
                    cfd = INVLD_SCKT;
                }
            }
        }

        #ifdef _WIN32
        if (!_kbhit())
            continue;
        #endif

        if (fgetws(command, sizeof(command), stdin) != NULL)  {
            command[wcslen(command) - 1] = L'\0';

            if (wcsstr(command, L"ECHO") != NULL)
                echo();
            else if (wcsstr(command, L"TIME") != NULL)
                server_time();
            else if (wcsstr(command, L"SETTINGS") != NULL) 
                settings_command(settings, command); 
            else if (wcsstr(command, L"QUIT") != NULL)
                break;
            wprintf(L"> ");
        }
    }

    #ifdef __linux__
    fcntl(STDIN_FILENO, F_SETFL, flags);
    #endif
    close(sfd);
}

int start_server(SCKT* sfd, int port) {

    *sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sfd == INVLD_SCKT)
        return 0;

    int opt = 1;
    #ifdef _WIN32
    setsockopt(*sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    #elif __linux__
    setsockopt(*sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif

    setup_keepalive(sfd);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(*sfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(*sfd);
        return 0;
    }

    if (listen(*sfd, 5) == -1) {
        close(*sfd);
        return 0;
    }

    #ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(*sfd, FIONBIO, &mode);
    #elif __linux__
    fcntl(*sfd, F_SETFL, O_NONBLOCK);
    #endif

    return 1;
}

int process_client(SCKT* cfd) {

    wchar_t* buffer = (wchar_t*)malloc(BUFFER_SIZE * sizeof(wchar_t));
    if (read_with_check_wstr(cfd, &buffer, BUFFER_SIZE, NULL) <= 0) {
        free(buffer);
        return 0;
    }

    if (wcsstr(buffer, L"UPLOAD") != NULL) {
        wprintf(L"\rКлиент начал загрузку файла.\n");
        wchar_t* file = buffer + 7;
        if (receive_data(cfd, file) == 0)
            wprintf(L"\rОшибка получения файла от клиента.\n> ");
    } else if (wcsstr(buffer, L"DOWNLOAD") != NULL) {
        wprintf(L"\rКлиент начал скачиваение файла.\n");
        wchar_t* file = buffer + 9;
        if (send_data(cfd, file) == 0)
            wprintf(L"\rОшибка отправки файла клиенту.\n> ");
    } else if (wcsstr(buffer, L"QUIT") != NULL) {
        wprintf(L"\rКлиент "CYAN"%ls"RESET" отключился.\n> ", current.client);
        free(buffer);
        return 1;
    }

    free(buffer);
    return 0;
}

int receive_data(SCKT* cfd, const wchar_t* file) {

    init_load_info_file(&current, file, NULL, 1);

    wchar_t* serverFilePath = get_file_path(settings->file_path, get_filename(file));

    size_t len = wcslen(serverFilePath) * sizeof(wchar_t) + 1;
    char* utf8_filename = (char*)malloc(len);
    wcstombs(utf8_filename, serverFilePath, len);

    FILE* f = fopen(utf8_filename, "rb+");
    if (f == NULL)
        f = fopen(utf8_filename, "wb");
    free(utf8_filename);

    int rat;
    while ((rat = read_with_check_long(cfd, &current.fileSize, f)) == 0);
    if (rat < 0)
        return 0;

    if (same_clients_files(&current, &last))
        copy_file(&current, &last, f);
    if (write_with_check_long(cfd, &current.processed, f) < 0)
        return 0;

    int rec; 
    double recTime = 0.0, packTime = 0.0;
    struct timeval start, end;
    LLINE* lline = init_lline(((double)current.processed / current.fileSize) * 100.0, current.fileSize);
    show_lline(lline);

    gettimeofday(&start, NULL);
    char* buffer = (char*)malloc(BUFFER_SIZE);
    while (current.processed < current.fileSize) {

        rec = read_with_check_str(cfd, &buffer, BUFFER_SIZE, f);
        if (rec == 0)
            continue;
        else if (rec < 0) {
            copy_info(&last, &current);
            return 0;
        }

        fwrite(buffer, sizeof(char), rec, f);
        current.processed += rec;
 
        gettimeofday(&end, NULL);
        packTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6;
        if (refresh_lline(lline, ((double)current.processed / current.fileSize) * 100.0, packTime) == 1) {
            recTime += packTime; 
            gettimeofday(&start, NULL);
        }
    }
    free_lline(lline);

    wprintf(L"\rПолучен файл от клиента. Скорость: "GREEN"%ls"RESET"; Время: "CYAN"%.2f с."RESET"\n> ", 
                    get_speed_stirng(get_speed(current.fileSize, 100.0, recTime)), recTime);
    copy_info(&last, &current);
    fclose(f);
    free(serverFilePath);
    free(buffer);
    return 1;
}

int send_data(SCKT* cfd, const wchar_t* file) {

    const wchar_t* serverFilePath = is_absolute_path(file) != 1 ? get_file_path(settings->file_path, file) : file;

    size_t len = wcslen(serverFilePath) * sizeof(wchar_t) + 1;
    char* utf8_filename = (char*)malloc(len);
    wcstombs(utf8_filename, serverFilePath, len);

    FILE* f = fopen(utf8_filename, "rb");
    free (utf8_filename);
    if (f == NULL)
        return 0;

    init_load_info_file(&current, file, f, 0);
    if (same_clients_files(&current, &last))
        copy_file(&current, &last, f);

    if (write_with_check_long(cfd, &current.fileSize, f) < 0)
        return 0;
    if (write_with_check_long(cfd, &current.processed, f) < 0)
        return 0;

    double new_percent = ((double)current.processed / current.fileSize) * 100.0, recTime = 0.0, packTime = 0.0;
    struct timeval start, end;
    LLINE* lline = init_lline(new_percent, current.fileSize);
    show_lline(lline);

    gettimeofday(&start, NULL);
    int read;
    char* buffer = (char*)malloc(BUFFER_SIZE);
    while (current.processed < current.fileSize && (read = fread(buffer, 1, BUFFER_SIZE, f))) {
        if (write_with_check_str(cfd, buffer, read, f) < 0) {
            copy_info(&last, &current);
            return 0;
        }
        current.processed += read;
        
        gettimeofday(&end, NULL);
        new_percent = ((double)current.processed / current.fileSize) * 100.0;
        packTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6;
        if(refresh_lline(lline, new_percent, packTime) == 1) {
            recTime += packTime;
            gettimeofday(&start, NULL);
        }
    }
    free_lline(lline);

    wprintf(L"\rФайл отправлен клиенту. Скорость: "GREEN"%ls"RESET"; Время: "CYAN"%.2f с."RESET"\n> ", 
            get_speed_stirng(get_speed(current.fileSize, 100.0, recTime)), recTime);
    copy_info(&last, &current);
    fclose(f);
    free(buffer);
    return 1;
}

void echo() {

    if (last.fileName[0] != L'\0')
        wprintf(L"Последняя команда: %ls. Файл: %ls\n", !last.download ? L"отправлен клиенту" : L"получен от клиента", last.fileName);
    else
        wprintf(L"Операций приёма/передачи не производилось.\n");
}

void server_time() {

    time_t now = time(NULL);
    struct tm* timeInfo = localtime(&now);

    wprintf(L"Текущее время: %02d.%02d.%4d %02d:%02d:%02d\n", 
        timeInfo->tm_mday, timeInfo->tm_mon + 1, timeInfo->tm_year + 1900,
        timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
}