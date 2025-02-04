#include "client.h"

FILE* logger;

void run(const char* server) {
    
    logger = start_log(LOG_FILE);
    log_message(logger, LOG_INFO, "Start client work");

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    char command[80];
    int cfd;
    start_client(&cfd, server);

    printf("> ");
    while (1) {

        check_connection(&cfd, logger, 1);

        if(fgets(command, sizeof(command), stdin)) {
            command[strlen(command) - 1] = '\0';
            if (strstr(command, "UPLOAD") != NULL) {
                upload(&cfd, command);
            }
            else if (strstr(command, "DOWNLOAD") != NULL) {
                download(&cfd, command);
            }
            else if (strcmp(command, "QUIT") == 0) {
                write_with_check(&cfd, "QUIT", sizeof("QUIT"), logger, NULL, 1);
                break;
            }
            printf("> ");
        }
    }

    log_message(logger, LOG_INFO, "Stop client work");
    fcntl(STDIN_FILENO, F_SETFL, flags);
    fclose(logger);
    close(cfd);
}

void start_client(int* cfd, const char* serverName) {

    struct hostent* server;
    struct sockaddr_in addr;
    
    server = gethostbyname(serverName);
    log_message(logger, LOG_INFO, "Get host by name");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    bcopy((char*)server->h_addr_list[0], (char*)&addr.sin_addr.s_addr, server->h_length);

    *cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*cfd == -1) {
        log_message(logger, LOG_CRITICAL, "Can't open socket");
        close(*cfd);
        exit(errno);
    }
    log_message(logger, LOG_INFO, "Open client socket");

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(*cfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    setup_keepalive(cfd);

    while (connect(*cfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        if (errno == ENOENT) {
            sleep(1);
            continue;
        } else {
            log_message(logger, LOG_CRITICAL, "Server connection error");
            close(*cfd);
            exit(errno);
        }
    }
    log_message(logger, LOG_INFO, "Connect to server");
}

void upload(int* cfd, const char* command) {

    log_message(logger, LOG_INFO, "Start upload file to server");
    char* buffer = (char*)malloc(80*sizeof(char));
    int fileSize = -1, sent = 0, bytesRead;
    const char* file = command + 7;

    FILE* f = fopen(file, "rb");
    if (f == NULL) {
        log_message(logger, LOG_CRITICAL, "Can't open file to send data to server");
        fclose(f);
        return;
    }

    write_with_check(cfd, command, strlen(command) + 1, logger, f, 1); // ###
    if (read_with_check_int(cfd, &fileSize, logger, f, 1) >= 0 && fileSize == -1) { // ###
        log_message(logger, LOG_CRITICAL, "Error while create file on server");
        fclose(f);
        return;
    }

    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    rewind(f);
    write_with_check_int(cfd, &fileSize, logger, f, 1); // ###

    int read;
    while (sent < fileSize && (read = fread(buffer, 1, 80, f))) {
        write_with_check(cfd, buffer, read, logger, f, 1);
        sent += read;
        // display persantage and amount of sent bytes
    }

    log_message(logger, LOG_INFO, "Data successfully sent to server");
    fclose(f);
    free(buffer);
}

void download(int* cfd, const char* command) {

    log_message(logger, LOG_INFO, "Start download data from server");

    char* buffer = (char*)malloc(80 * sizeof(char));
    int fileSize = -1, received = 0;
    const char* file = command + 9;

    write_with_check(cfd, command, strlen(command) + 1, logger, NULL, 1); // ###
    if (read_with_check_int(cfd, &fileSize, logger, NULL, 1) >= 0 && fileSize == -1) { // ###
        log_message(logger, LOG_ERROR, "No such file on server");
        return;
    }

    FILE* f = fopen(file, "wb");
    if (f == NULL) {
        log_message(logger, LOG_ERROR, "Can't create file to receive data from server");
        fclose(f);
        write_with_check_int(cfd, &fileSize, logger, f, 1);
        return;
    }

    read_with_check_int(cfd, &fileSize, logger, f, 1); // ###

    int rec;
    while (received < fileSize && (rec = read_with_check(cfd, &buffer, 80, logger, f, 1))) { // ###
        fwrite(buffer, sizeof(char), rec, f);
        received += rec;
    }
        // print percantage and amount of bytes

    log_message(logger, LOG_INFO, "Server's data successfully received");
    fclose(f);
    free(buffer);
}