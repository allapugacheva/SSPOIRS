#include "server.h"

SETTINGS* settings;
LOAD_INFO current, last;

void run(int port) {

    settings = init_settings();

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    char command[BUFFER_SIZE];
    int sfd, cfd = -1;
    if (!start_server(&sfd, port)) {
        printf("Error while start server.\n");
        exit(errno);
    }
    
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    printf("> ");
    while (1) {
        
        if (check_connection(&cfd) == 0)
            printf("\rLost connection with client %s\n> ", current.client);

        if (cfd == -1) {
            cfd = accept(sfd, (struct sockaddr*)&clientAddr, &clientLen);
            if (cfd == -1 && !(errno == EWOULDBLOCK || errno == EAGAIN)) {
                printf("\rAccept client error.\n");
                close(sfd);
                close(cfd);
                exit(errno);
            } else if (cfd != -1) {
                char clientIp[BUFFER_SIZE];
                inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, BUFFER_SIZE);
                init_load_info_client(&current, clientIp);
                printf("\rAccept client connection: %s\n> ", clientIp);
            }
        } else
            if (process_client(&cfd)) {
                close(cfd);
                cfd = -1;
            }

        if (fgets(command, sizeof(command), stdin))  {
            command[strlen(command) - 1] = '\0';

            if (strstr(command, "ECHO") != NULL)
                echo();
            else if (strstr(command, "TIME") != NULL)
                server_time();
            else if (strstr(command, "SETTINGS") != NULL) 
                settings_command(settings, command); 
            else if (strstr(command, "QUIT") != NULL)
                break;
            printf("> ");
        }
    }

    fcntl(STDIN_FILENO, F_SETFL, flags);
    close(sfd);
}

int start_server(int* sfd, int port) {

    *sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sfd == -1)
        return 0;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 300000;
    setsockopt(*sfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(*sfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    int snd_buf_size = BUFFER_SIZE;
    setsockopt(*sfd, SOL_SOCKET, SO_SNDBUF, &snd_buf_size, sizeof(snd_buf_size));
    setsockopt(*sfd, SOL_SOCKET, SO_RCVBUF, &snd_buf_size, sizeof(snd_buf_size));

    int opt = 1;
    setsockopt(*sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

    fcntl(*sfd, F_SETFL, O_NONBLOCK);
    return 1;
}

int process_client(int* cfd) {

    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = 0; 
    timeout.tv_usec = 0; 

    FD_ZERO(&readfds);
    FD_SET(*cfd, &readfds);

    int ready = select(*cfd + 1, &readfds, NULL, NULL, &timeout);
    if (!(ready > 0 && FD_ISSET(*cfd, &readfds)))
        return 0;

    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    if (read_with_check_str(cfd, &buffer, BUFFER_SIZE, NULL) == -2) {
        free(buffer);
        return 0;
    }

    if (strstr(buffer, "UPLOAD") != NULL) {
        printf("\rClient start uploading file.\n");
        char* file = buffer + 7;
        if (receive_data(cfd, file) == 0)
            printf("\rError while receive file from client.\n");
    } else if (strstr(buffer, "DOWNLOAD") != NULL) {
        printf("\rClient start downloading file.\n");
        char* file = buffer + 9;
        if (send_data(cfd, file) == 0)
            printf("\rError while send file to client.\n");
    } else if (strstr(buffer, "QUIT") != NULL) {
        printf("\rClient %s disconnected.\n> ", current.client);
        free(buffer);
        return 1;
    }

    free(buffer);
    return 0;
}

int receive_data(int* cfd, const char* file) {

    init_load_info_file(&current, file, NULL, 1);
    int same = same_clients_files(&current, &last);

    char* serverFilePath = get_file_path(settings->file_path, get_filename(file));
    FILE* f = fopen(serverFilePath, same ? "ab" : "wb");
    if (f == NULL)
        return 0;

    if (read_with_check_long(cfd, &current.fileSize, f) == -2)
        return 0;
    if (same)
        copy_file(&current, &last, f);
    if (write_with_check_long(cfd, &current.processed, f) == -2)
        return 0;

    int rec; 
    double recTime = 0.0, packTime = 0.0;
    struct timeval start, end;
    LLINE* lline = init_lline(((double)current.processed / current.fileSize) * 100.0, current.fileSize);
    show_lline(lline);

    gettimeofday(&start, NULL);
    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    while (current.processed < current.fileSize && (rec = read_with_check_str(cfd, &buffer, BUFFER_SIZE, f))) {
        if (rec == -2) {
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

    printf("\rReceive file from client. Speed: "GREEN"%s"RESET"; Time: "CYAN"%.2fs"RESET"\n> ", 
                    get_speed_stirng(get_speed(current.fileSize, 100.0, recTime)), recTime);
    copy_info(&last, &current);
    fclose(f);
    free(serverFilePath);
    free(buffer);
    return 1;
}

int send_data(int* cfd, const char* file) {

    const char* serverFilePath = is_absolute_path(file) != 1 ? get_file_path(settings->file_path, file) : file;
    FILE* f = fopen(serverFilePath, "rb");
    if (f == NULL)
        return 0;

    init_load_info_file(&current, file, f, 0);
    if (same_clients_files(&current, &last))
        copy_file(&current, &last, f);

    if (write_with_check_long(cfd, &current.fileSize, f) == -2)
        return 0;
    if (write_with_check_long(cfd, &current.processed, f) == -2)
        return 0;

    double new_percent = ((double)current.processed / current.fileSize) * 100.0, recTime = 0.0, packTime = 0.0;
    struct timeval start, end;
    LLINE* lline = init_lline(new_percent, current.fileSize);
    show_lline(lline);

    gettimeofday(&start, NULL);
    int read;
    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    while (current.processed < current.fileSize && (read = fread(buffer, 1, BUFFER_SIZE, f))) {
        if (write_with_check_str(cfd, buffer, read, f) == -2) {
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

    printf("\rSuccessfully sent data to client. Speed: "GREEN"%s"RESET"; Time: "CYAN"%.2fs"RESET"\n> ", 
            get_speed_stirng(get_speed(current.fileSize, 100.0, recTime)), recTime);
    copy_info(&last, &current);
    fclose(f);
    free(buffer);
    return 1;
}

void echo() {

    if (last.fileName[0] != '\0')
        printf("Last operation: %s. File: %s\n", !last.download ? "send to client" : "receive from client", last.fileName);
    else
        printf("No file processed before\n");
}

void server_time() {

    time_t now = time(NULL);
    struct tm* timeInfo = localtime(&now);

    printf("%02d.%02d.%4d %02d:%02d:%02d\n", 
        timeInfo->tm_mday, timeInfo->tm_mon + 1, timeInfo->tm_year + 1900,
        timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
}