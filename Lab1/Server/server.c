#include "server.h"

char* lastFile = NULL;
FILE* logger;

void run() {

    logger = start_log(LOG_FILE);
    log_message(logger, LOG_INFO, "Start server work");

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    char command[10];

    int sfd, cfd = -1;
    start_server(&sfd);
    
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    printf("> ");
    while (1) {
        
        check_connection(&cfd, logger, 0);

        if (cfd == -1) {
            cfd = accept(sfd, (struct sockaddr*)&clientAddr, &clientLen);
            if (cfd == -1 && !(errno == EWOULDBLOCK || errno == EAGAIN)) {
                log_message(logger, LOG_CRITICAL, "Accept client error");
                close(sfd);
                close(cfd);
                exit(errno);
            } else if (cfd != -1)
                log_message(logger, LOG_INFO, "Accept client connection"); // write also client addr??
        } else
            if (process_client(&cfd)) {
                log_message(logger, LOG_INFO, "Client disconnected");
                close(cfd);
                cfd = -1;
            }

        if (fgets(command, sizeof(command), stdin))  {
            command[strlen(command) - 1] = '\0';
            if (strcmp(command, "ECHO") == 0)
                echo();
            else if (strcmp(command, "TIME") == 0)
                server_time();
            else if (strcmp(command, "QUIT") == 0)
                break;
            printf("> ");
        }
    }

    log_message(logger, LOG_INFO, "Stop server work");
    fcntl(STDIN_FILENO, F_SETFL, flags);
    fclose(logger);
    close(sfd);
}

void start_server(int* sfd) {

    *sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sfd == -1) {
        log_message(logger, LOG_CRITICAL, "Can't open socket");
        close(*sfd);
        exit(errno);
    }
    log_message(logger, LOG_INFO, "Open socket");

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(*sfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int opt = 1;
    if(setsockopt(*sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        log_message(logger, LOG_CRITICAL, "Can't set socket options");
        close (*sfd);
        exit(errno);
    }

    setup_keepalive(sfd);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(*sfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        log_message(logger, LOG_CRITICAL, "Can't bind socket");
        close(*sfd);
        exit(errno);
    }
    log_message(logger, LOG_INFO, "Bing server");

    if (listen(*sfd, 5) == -1) {
        log_message(logger, LOG_CRITICAL, "Can't start listen");
        close(*sfd);
        exit(errno);
    }
    log_message(logger, LOG_INFO, "Server start to listen");

    fcntl(*sfd, F_SETFL, O_NONBLOCK);
}

int process_client(int* cfd) {

    char* buffer = (char*)malloc(80 * sizeof(char));
    if (read_with_check(cfd, &buffer, 80, logger, NULL, 0) == -2) {
        free(buffer);
        return 0;
    }
    if (strstr(buffer, "UPLOAD") != NULL) {
        char* file = buffer + 7;
        receive_data(cfd, file);
    } else if (strstr(buffer, "DOWNLOAD") != NULL) {
        char* file = buffer + 9;
        send_data(cfd, file);
    } else if (strstr(buffer, "QUIT") != NULL) {
        free(buffer);
        return 1;
    }
    free(buffer);
    return 0;
}

void receive_data(int* cfd, const char* file) {

    log_message(logger, LOG_INFO, "Start receive client data");

    char* buffer = (char*)malloc(80 * sizeof(char));
    int fileSize = -1, received = 0;

    FILE* f = fopen(file, "wb");
    if (f == NULL) {
        log_message(logger, LOG_ERROR, "Can't create file to receive data from client");
        write_with_check_int(cfd, &fileSize, logger, f, 0);
        return;
    }

    if(read_with_check_int(cfd, &fileSize, logger, f, 0) == -2) // ###
        return;

    int rec;
    while (received < fileSize && (rec = read_with_check(cfd, &buffer, 80, logger, f, 0))) { // ###
        if(rec == -2)
            return;
        fwrite(buffer, sizeof(char), rec, f);
        received += rec;
    }
        // print percantage and amount of bytes

    log_message(logger, LOG_INFO, "Client's data successfully received");
    fclose(f);
    free(buffer);
}

void send_data(int* cfd, const char* file) {

    log_message(logger, LOG_INFO, "Start send data to client");
    // process lost connection

    char* buffer = (char*)malloc(80 * sizeof(char));
    int fileSize = -1, sent = 0, bytesRead;

    FILE* f = fopen(file, "rb");
    if (f == NULL) {
        log_message(logger, LOG_ERROR, "Can't open file to send data to client");
        write_with_check_int(cfd, &fileSize, logger, f, 0); // ###
        return;
    }

    int ret;
    if ((ret = read_with_check_int(cfd, &fileSize, logger, f, 0)) == -2)
        return;
    if (fileSize == -1 && ret != -1) { // ###
        log_message(logger, LOG_ERROR, "Can't open file on client");
        fclose(f);
        return;
    }

    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    rewind(f);
    if (write_with_check_int(cfd, &fileSize, logger, f, 0) == -2) // ###
        return;
        
    int read;
    while (sent < fileSize && (read = fread(buffer, 1, 80, f))) {
        if (write_with_check(cfd, buffer, read, logger, f, 0) == -2) // ###
            return;
        sent += read;
        // display persantage and amount of sent bytes
    }
    log_message(logger, LOG_INFO, "Data successfully sent to client");
    fclose(f);
    free(buffer);
}

void echo() {
    if (lastFile != NULL)
        printf("Last processed file: %s\n", lastFile);
    else
        printf("No file processed before\n");
    log_message(logger, LOG_INFO, "Process command ECHO");
}

void server_time() {
    time_t now = time(NULL);
    struct tm* timeInfo = localtime(&now);

    printf("%02d.%02d.%4d %2d:%2d:%2d\n", 
        timeInfo->tm_mday, timeInfo->tm_mon + 1, timeInfo->tm_year + 1900,
        timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
    log_message(logger, LOG_INFO, "Process command TIME");
}