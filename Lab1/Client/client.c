#include "client.h"

FILE* logger;

void run(const char* server) {
    
    logger = start_log(LOG_FILE);
    log_message(logger, LOG_INFO, "Start client work");

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

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

            if(!check_connection(&cfd, logger, 1))
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
                else if (strcmp(command, "QUIT") == 0) {
                    write_with_check(&cfd, "QUIT", sizeof("QUIT"), logger, NULL, 1);
                    work = 0;
                    break;
                }
                printf("> ");
            }
        }

        if (work) {
            printf("\rDo you want to reconnect to server? [y/n]: ");
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
    timeout.tv_sec = 3;
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
    int fileSize = -1, sent = 0, bytesRead, ret;
    const char* file = command + 7;

    FILE* f = fopen(file, "rb");
    if (f == NULL) {
        printf("\rCan't open file to send data to server\n");
        log_message(logger, LOG_ERROR, "Can't open file to send data to server");
        return -1;
    }

    if (write_with_check(cfd, command, strlen(command) + 1, logger, f, 1) == -2)
        return 0;
    if ((ret = read_with_check_int(cfd, &fileSize, logger, f, 1)) >= 0 && fileSize == -1) {
        printf("\rCan't create file on server\n");
        log_message(logger, LOG_ERROR, "Error while create file on server");
        fclose(f);
        return -1;
    }
    if (ret == -2)
        return 0;

    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    rewind(f);
    if (write_with_check_int(cfd, &fileSize, logger, f, 1) == -2)
        return 0;
    
    if (read_with_check_int(cfd, &sent, logger, f, 1) == -2)
        return 0;
    if (sent != 0)
        fseek(f, sent, SEEK_SET);

    int read;
    while (sent < fileSize && (read = fread(buffer, 1, BUFFER_SIZE, f))) {
        if (write_with_check(cfd, buffer, read, logger, f, 1) == -2)
            return 0;
        sent += read;
        // display persantage and amount of sent bytes
    }

    printf("\rFile successfully sent to server\n");
    log_message(logger, LOG_INFO, "Data successfully sent to server");
    fclose(f);
    free(buffer);
    return 1;
}

int download(int* cfd, const char* command) {

    log_message(logger, LOG_INFO, "Start download data from server");

    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    int fileSize = -1, received = 0;
    const char* file = command + 9;

    if (write_with_check(cfd, command, strlen(command) + 1, logger, NULL, 1) == -2)
        return 0;
    int ret;
    if ((ret = read_with_check_int(cfd, &fileSize, logger, NULL, 1)) >= 0 && fileSize == -1) {
        printf("\rNo such file on server\n");
        log_message(logger, LOG_ERROR, "No such file on server");
        return -1;
    }
    if (ret == -2)
        return 0;

    FILE* f = fopen(file, "wb");
    if (f == NULL) {
        printf("\rCan't create file to receive data from server\n");
        log_message(logger, LOG_ERROR, "Can't create file to receive data from server");
        write_with_check_int(cfd, &fileSize, logger, NULL, 1);
        return -1;
    }

    if (read_with_check_int(cfd, &fileSize, logger, f, 1) == -2)
        return 0;
    if (read_with_check_int(cfd, &received, logger, f, 1) == -2)
        return 0;
    if (received != 0)
        fseek(f, received, SEEK_SET);

    int rec;
    while (received < fileSize && (rec = read_with_check(cfd, &buffer, BUFFER_SIZE, logger, f, 1))) {
        if (rec == -2)
            return 0;
        fwrite(buffer, sizeof(char), rec, f);
        received += rec;
    }

    printf("\rSuccessfully receive data from server\n");
    log_message(logger, LOG_INFO, "Server's data successfully received");
    fclose(f);
    free(buffer);
    return 1;
}