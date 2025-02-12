#include "client.h"

FILE* logger;
SETTINGS* settings;

void run(const char* server) {
    
    logger = start_log(LOG_FILE);
    log_message(logger, LOG_INFO, "Start client work");

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    settings = init_settings();
    int cfd;
    char command[BUFFER_SIZE];

    int work = 1;
    while(work) {

        if (!start_client(&cfd, server)) {
            printf("\rError. Check log\n");
            exit(errno);
        }
        printf("> ");
        while (1) {

            if(!check_connection(&cfd, logger))
                break;

            if(fgets(command, sizeof(command), stdin)) {
                command[strlen(command) - 1] = '\0';

                if (strstr(command, "UPLOAD") != NULL) {
                    if (upload(&cfd, command) == 0)
                        break;
                }
                else if (strstr(command, "DOWNLOAD") != NULL) {
                    if (download(&cfd, command) == 0)
                        break;
                }
                else if(strstr(command, "SETTINGS") != NULL)
                    settings_command(command); 
                else if (strcmp(command, "QUIT") == 0) {
                    write_with_check(&cfd, "QUIT", sizeof("QUIT"), logger, NULL);
                    work = 0;
                    break;
                }
                printf("> ");
            }
        }

        if (work) {
            printf("\rLose connection to server. Do you want to reconnect? [y/n]: ");
            unsigned char ch;
            while((ch = getchar()) != 'y' && ch != 'n');
            fflush(stdin);
            if(ch != 'y')
                break;
        }
    }
    log_message(logger, LOG_INFO, "Stop client work");
    fcntl(STDIN_FILENO, F_SETFL, flags);
    fclose(logger);
    close(cfd);
}

int start_client(int* cfd, const char* serverName) {

    *cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*cfd == -1) {
        log_message(logger, LOG_CRITICAL, "Can't open socket");
        close(*cfd);
        return 0;
    }
    log_message(logger, LOG_INFO, "Open client socket");

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(*cfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    setup_keepalive(cfd);

    struct hostent* server;
    struct sockaddr_in addr;
    
    server = gethostbyname(serverName);
    log_message(logger, LOG_INFO, "Get host by name");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    bcopy((char*)server->h_addr_list[0], (char*)&addr.sin_addr.s_addr, server->h_length);

    int tries = 0, maxTries = 10;
    while (connect(*cfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        if (errno == ENOENT) {
            if (++tries >= maxTries) {
                log_message(logger, LOG_CRITICAL, "Server is not available");
                close(*cfd);
                return 0;
            }
            sleep(1);
            continue;
        } else {
            log_message(logger, LOG_CRITICAL, "Server connection error");
            close(*cfd);
            return 0;
        }
    }

    log_message(logger, LOG_INFO, "Connect to server");
    return 1;
}

int upload(int* cfd, const char* command) {

    log_message(logger, LOG_INFO, "Start upload file to server");

    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    long fileSize = -1, sent = 0, read;

    const char* filePath = command + 7;
    if(is_absolute_path(filePath) != 1)
        filePath = get_file_path(settings->file_path, filePath);

    FILE* f = fopen(filePath, "rb");
    if (f == NULL) {
        printf("\rCan't open file to send data to server\n");
        log_message(logger, LOG_ERROR, "Can't open file to send data to server");
        return -1;
    }

    if (write_with_check(cfd, command, strlen(command) + 1, logger, f) == -2)
        return 0;
    if ((read = read_with_check_int(cfd, &fileSize, logger, f)) >= 0 && fileSize == -1) {
        printf("\rCan't create file on server\n");
        log_message(logger, LOG_ERROR, "Error while create file on server");
        fclose(f);
        return -1;
    }
    if (read == -2)
        return 0;

    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    rewind(f);
    if (write_with_check_int(cfd, &fileSize, logger, f) == -2)
        return 0;
    
    if (read_with_check_int(cfd, &sent, logger, f) == -2)
        return 0;
    if (sent != 0)
        fseek(f, sent, SEEK_SET);

    double new_percent = ((double)sent / fileSize) * 100.0, recTime = 0.0, packTime = 0.0;
    struct timeval start, end;
    LLINE* lline = init_lline(new_percent, fileSize);
    show_lline(lline);

    gettimeofday(&start, NULL);
    while (sent < fileSize && (read = fread(buffer, 1, BUFFER_SIZE, f))) {
        if (write_with_check(cfd, buffer, read, logger, f) == -2)
            return 0;
        sent += read;
        
        gettimeofday(&end, NULL);
        new_percent = ((double)sent / fileSize) * 100.0;
        packTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6;
        if(refresh_lline(lline, new_percent, packTime) == 1) {
            recTime += packTime;
            gettimeofday(&start, NULL);
        }
    }
    free_lline(lline);

    printf("\rFile successfully sent to server. Speed: "GREEN"%s"RESET"; Time: "CYAN"%.2fs"RESET"\n", 
                                                    get_speed_stirng(get_speed(fileSize, 100.0, recTime)), recTime);
    log_message(logger, LOG_INFO, "Data successfully sent to server");
    fclose(f);
    free(buffer);
    return 1;
}

int download(int* cfd, const char* command) {

    log_message(logger, LOG_INFO, "Start download data from server");

    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    long fileSize = -1, received = 0;
    int rec;

    if (write_with_check(cfd, command, strlen(command) + 1, logger, NULL) == -2)
        return 0;
    if ((rec = read_with_check_int(cfd, &fileSize, logger, NULL)) >= 0 && fileSize == -1) {
        printf("\rNo such file on server\n");
        log_message(logger, LOG_ERROR, "No such file on server");
        return -1;
    }
    if (rec == -2)
        return 0;

    char* filePath = get_file_path(settings->file_path, get_filename(command + 9));

    FILE* f = fopen(filePath, "wb");
    if (f == NULL) {
        printf("\rCan't create file to receive data from server\n");
        log_message(logger, LOG_ERROR, "Can't create file to receive data from server");
        write_with_check_int(cfd, &fileSize, logger, NULL);
        return -1;
    }

    if (read_with_check_int(cfd, &fileSize, logger, f) == -2)
        return 0;
    if (read_with_check_int(cfd, &received, logger, f) == -2)
        return 0;
    if (received != 0)
        fseek(f, received, SEEK_SET);

    double new_percent = ((double)received / fileSize) * 100.0, recTime = 0.0, packTime = 0.0;
    struct timeval start, end;
    LLINE* lline = init_lline(new_percent, fileSize);
    show_lline(lline);

    gettimeofday(&start, NULL);
    while (received < fileSize && (rec = read_with_check(cfd, &buffer, BUFFER_SIZE, logger, f))) {
        if (rec == -2)
            return 0;
        fwrite(buffer, sizeof(char), rec, f);
        received += rec;

        gettimeofday(&end, NULL);
        new_percent = ((double)received / fileSize) * 100.0;
        packTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6;
        if(refresh_lline(lline, new_percent, packTime) == 1) {
            recTime += packTime;
            gettimeofday(&start, NULL);
        }
    }

    free_lline(lline);

    printf("\rSuccessfully receive data from server. Speed: "GREEN"%s"RESET"; Time: "CYAN"%.2fs"RESET"\n", 
                                                    get_speed_stirng(get_speed(fileSize, 100.0, recTime)), recTime);
    log_message(logger, LOG_INFO, "Server's data successfully received");
    fclose(f);
    free(filePath);
    free(buffer);
    return 1;
}

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