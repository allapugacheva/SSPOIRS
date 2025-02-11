#include "server.h"

FILE *logger;
SETTINGS* settings;
LOAD_INFO current, last;

void run() {

    logger = start_log(LOG_FILE);
    log_message(logger, LOG_INFO, "Start server work");

    settings = init_settings();

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    char command[BUFFER_SIZE];

    int sfd, cfd = -1;
    if (!start_server(&sfd)) {
        printf("\rError. Check log\n");
        exit(errno);
    }
    
    vl_server_start();

    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    printf("> ");
    while (1) {
        
        check_connection(&cfd, logger);

        if (cfd == -1) {
            cfd = accept(sfd, (struct sockaddr*)&clientAddr, &clientLen);
            if (cfd == -1 && !(errno == EWOULDBLOCK || errno == EAGAIN)) {
                log_message(logger, LOG_CRITICAL, "Accept client error");
                close(sfd);
                close(cfd);
                exit(errno);
            } else if (cfd != -1) {
                log_message(logger, LOG_INFO, "Accept client connection");
                char clientIp[BUFFER_SIZE];
                inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, BUFFER_SIZE);
                init_load_info_client(&current, clientIp);
                printf("\rAccept client connection: %s\n> ", clientIp);
            }
        } else
            if (process_client(&cfd)) {
                log_message(logger, LOG_INFO, "Client disconnected");
                close(cfd);
                cfd = -1;
            }

        if (fgets(command, sizeof(command), stdin))  {
            //printf("FUCK1\n");
            command[strlen(command) - 1] = '\0';
            if (strcmp(command, "ECHO") == 0)
                echo();
            else if (strcmp(command, "TIME") == 0) {
                server_time();
                //printf("FUCK2\n");
            }
            else if(strstr(command, "SETTINGS") != 0) 
                settings_command(command); 
            else if (strcmp(command, "QUIT") == 0)
                break;
            //printf("FUCK3\n");
            printf("> ");
        }
    }

    log_message(logger, LOG_INFO, "Stop server work");
    fcntl(STDIN_FILENO, F_SETFL, flags);
    fclose(logger);
    close(sfd);
}

int start_server(int* sfd) {

    *sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sfd == -1) {
        log_message(logger, LOG_CRITICAL, "Can't open socket");
        close(*sfd);
        return 0;
    }
    log_message(logger, LOG_INFO, "Open socket");

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(*sfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int opt = 1;
    setsockopt(*sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    setup_keepalive(sfd);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(*sfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        log_message(logger, LOG_CRITICAL, "Can't bind socket");
        close(*sfd);
        return 0;
    }
    log_message(logger, LOG_INFO, "Bind server");

    if (listen(*sfd, 5) == -1) {
        log_message(logger, LOG_CRITICAL, "Can't start listen");
        close(*sfd);
        return 0;
    }
    log_message(logger, LOG_INFO, "Server start to listen");

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
    if (read_with_check(cfd, &buffer, BUFFER_SIZE, logger, NULL) == -2) {
        free(buffer);
        return 0;
    }

    if (strstr(buffer, "UPLOAD") != NULL) {
        printf("\rClient start uploading file\n");
        char* file = buffer + 7;
        receive_data(cfd, file);
    } else if (strstr(buffer, "DOWNLOAD") != NULL) {
        printf("\rClient start downloading file\n");
        char* file = buffer + 9;
        send_data(cfd, file);
    } else if (strstr(buffer, "QUIT") != NULL) {
        printf("\rClient disconnected\n> ");
        free(buffer);
        return 1;
    }
    free(buffer);
    return 0;
}

void receive_data(int* cfd, const char* file) {
    log_message(logger, LOG_INFO, "Start receive client data");

    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    int fileSize = -1, received = 0;

    init_load_info_file(&current, file, NULL, 1);
    int same = same_clients_files(&current, &last);
    char* fileName = get_filename(file);
    char* serverFilePath = get_file_path(settings->file_path, fileName);
    FILE* f = fopen(serverFilePath, same ? "ab" : "wb");
    if (f == NULL) {
        printf("\rCan't create file to receive data from client\n> ");
        log_message(logger, LOG_ERROR, "Can't create file to receive data from client");
        write_with_check_int(cfd, &fileSize, logger, NULL);
        return;
    }

    if (read_with_check_int(cfd, &current.fileSize, logger, f) == -2)
        return;
    if (same)
        copy_file(&current, &last, f);
    if (write_with_check_int(cfd, &current.processed, logger, f) == -2)
        return;
    vl_input_message();

    int rec; double new_percent = ((double)current.processed / current.fileSize) * 100.0;
    struct timeval start, end; double recTime = 0.0, packTime = 0.0;
    LLINE* lline = init_lline(new_percent, current.fileSize);
    show_lline(lline);

    gettimeofday(&start, NULL);
    while (current.processed < current.fileSize && (rec = read_with_check(cfd, &buffer, BUFFER_SIZE, logger, f))) {
        if(rec == -2)
            return;
        fwrite(buffer, sizeof(char), rec, f);
        current.processed += rec;

        gettimeofday(&end, NULL);
        new_percent = ((double)current.processed / current.fileSize) * 100.0;
        packTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6;
        if(refresh_lline(lline, new_percent, packTime) == 1) {
            recTime += packTime;
            gettimeofday(&start, NULL);
        }
    }
    free_lline(lline);

    char* speedString = get_speed_stirng(get_speed(current.fileSize, 100.0, recTime));
    printf("\rReceive file from client. Speed: "GREEN"%s"RESET"; Time: "CYAN"%.2fs"RESET"\n> ", speedString, recTime);
    log_message(logger, LOG_INFO, "Client's data successfully received");
    fclose(f);
    free(serverFilePath);
    free(fileName);
    free(buffer);
    free(speedString);
    copy_info(&last, &current);
    vl_task_success();
}

void send_data(int* cfd, const char* file) {

    log_message(logger, LOG_INFO, "Start send data to client");

    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    int fileSize = -1, sent = 0, bytesRead;

    char* serverFilePath = (char*)file;
    if(is_absolute_path(file) != 1) {
        serverFilePath = get_file_path(settings->file_path, file);
    }

    FILE* f = fopen(serverFilePath, "rb");
    if (f == NULL) {
        printf("\rCan't open file to send data to client\n> ");
        log_message(logger, LOG_ERROR, "Can't open file to send data to client");
        write_with_check_int(cfd, &fileSize, logger, NULL);
        free(buffer);
        return;
    }

    int ret;
    if ((ret = read_with_check_int(cfd, &current.fileSize, logger, f)) == -2)
        return;
    if (current.fileSize == -1 && ret != -1) {
        printf("\rCan't open file on client\n> ");
        log_message(logger, LOG_ERROR, "Can't open file on client");
        fclose(f);
        free(buffer);
        return;
    }

    init_load_info_file(&current, file, f, 0);
    if (same_clients_files(&current, &last))
        copy_file(&current, &last, f);

    if (write_with_check_int(cfd, &current.fileSize, logger, f) == -2)
        return;
    if (write_with_check_int(cfd, &current.processed, logger, f) == -2)
        return;

    int read; double new_percent = ((double)current.processed / current.fileSize) * 100.0;
    struct timeval start, end; double recTime = 0.0, packTime = 0.0;
    LLINE* lline = init_lline(new_percent, current.fileSize);
    show_lline(lline);

    gettimeofday(&start, NULL);
    while (current.processed < current.fileSize && (read = fread(buffer, 1, BUFFER_SIZE, f))) {
        if (write_with_check(cfd, buffer, read, logger, f) == -2)
            return;
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

    char* speedString = get_speed_stirng(get_speed(current.fileSize, 100.0, recTime));
    printf("\rSent data to client. Speed: "GREEN"%s"RESET"; Time: "CYAN"%.2fs"RESET"\n> ", speedString, recTime);
    log_message(logger, LOG_INFO, "Data successfully sent to client");
    fclose(f);
    free(buffer);
    free(speedString);
    copy_info(&last, &current);
}

void echo() {
    if (last.fileName[0] != '\0')
        printf("Last operation: %s. File: %s\n", !last.download ? "send to client" : "receive from client", last.fileName);
    else
        printf("No file processed before\n");
    log_message(logger, LOG_INFO, "Process command ECHO");
}

void server_time() {
    time_t now = time(NULL);
    struct tm* timeInfo = localtime(&now);

    printf("%02d.%02d.%4d %02d:%02d:%02d\n", 
        timeInfo->tm_mday, timeInfo->tm_mon + 1, timeInfo->tm_year + 1900,
        timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
    log_message(logger, LOG_INFO, "Process command TIME");
}

//__________________ SETTINGS _________________________

void settings_command(char* command) {
    if(strstr(command, ".path") != 0) {
        char* start_i = strchr(command, ' ');
        if(start_i == NULL) 
            return;

        int last_i = strlen(command) - 1;

        while(*(++start_i) == ' ');
 
        char dir[MAX_PATH_SIZE];
        if(*start_i == '"' && command[last_i] == '"') {
            start_i++; last_i--; 
            strncpy(dir, start_i, strlen(start_i));
            dir[strlen(start_i) - 1] = '\0';
        } else { 
            strcpy(dir, start_i);
        }
        
        settings_cmd(settings, SET_PATH, dir);

    } else if(strcmp(command, "SETTINGS") == 0) { 
        settings_cmd(settings, SETTINGS_LIST, NULL);
    }
}