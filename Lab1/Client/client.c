#include "client.h"

SETTINGS* settings;

void run(const char* server, int port) {

    settings = init_settings();
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    int cfd;
    char command[BUFFER_SIZE];

    printf("> ");
    int work = 1;
    while(work) {

        if (!start_client(&cfd, server, port)) {
            printf("\rError while start client.\n");
            exit(errno);
        }
        while (1) {

            if (!check_connection(&cfd))
                break;

            if (fgets(command, sizeof(command), stdin)) {
                command[strlen(command) - 1] = '\0';

                if (strstr(command, "UPLOAD") != NULL) {
                    if (upload(&cfd, command) == 0) {
                        printf("\rError while executing command UPLOAD.\n");
                        break;
                    }
                }
                else if (strstr(command, "DOWNLOAD") != NULL) {
                    if (download(&cfd, command) == 0) {
                        printf("\rError while executing command UPLOAD.\n");
                        break;
                    }
                }
                else if (strstr(command, "SETTINGS") != NULL)
                    settings_command(settings, command); 
                else if (strstr(command, "QUIT") != NULL) {
                    write_with_check_str(&cfd, "QUIT", sizeof("QUIT"), NULL);
                    work = 0;
                    break;
                }
                printf("> ");
            }
        }

        if (work) {
            printf("\rLost connection with server. Do you want to reconnect? [y/n]: ");
            unsigned char ch;
            while((ch = getchar()) != 'y' && ch != 'n');
            fflush(stdin);
            if (ch != 'y')
                break;
        }
    }

    fcntl(STDIN_FILENO, F_SETFL, flags);
    close(cfd);
}

int start_client(int* cfd, const char* serverName, int port) {

    *cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*cfd == -1) {
        close(*cfd);
        return 0;
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;
    setsockopt(*cfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(*cfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    int snd_buf_size = BUFFER_SIZE;
    setsockopt(*cfd, SOL_SOCKET, SO_SNDBUF, &snd_buf_size, sizeof(snd_buf_size));
    setsockopt(*cfd, SOL_SOCKET, SO_RCVBUF, &snd_buf_size, sizeof(snd_buf_size));

    setup_keepalive(cfd);

    struct hostent* server;
    struct sockaddr_in addr;
    server = gethostbyname(serverName);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    bcopy((char*)server->h_addr_list[0], (char*)&addr.sin_addr.s_addr, server->h_length);

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

    return 1;
}

int upload(int* cfd, const char* command) {

    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    long fileSize = -1, sent = 0, read;

    const char* filePath = command + 7;
    if (is_absolute_path(filePath) != 1)
        filePath = get_file_path(settings->file_path, filePath);

    FILE* f = fopen(filePath, "rb");
    if (f == NULL)
        return -1;

    if (write_with_check_str(cfd, command, strlen(command) + 1, f) == -2)
        return 0;

    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    rewind(f);
    if (write_with_check_long(cfd, &fileSize, f) == -2)
        return 0;

    if (read_with_check_long(cfd, &sent, f) == -2)
        return 0;
    if (sent != 0)
        fseek(f, sent, SEEK_SET);

    double recTime = 0.0, packTime = 0.0;
    struct timeval start, end;
    LLINE* lline = init_lline(((double)sent / fileSize) * 100.0, fileSize);
    show_lline(lline);

    gettimeofday(&start, NULL);
    while (sent < fileSize && (read = fread(buffer, 1, BUFFER_SIZE, f))) {
        if (write_with_check_str(cfd, buffer, read, f) == -2)
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

    printf("\rFile successfully sent to server. Speed: "GREEN"%s"RESET"; Time: "CYAN"%.2fs"RESET"\n", 
                                    get_speed_stirng(get_speed(fileSize, 100.0, recTime)), recTime);
    fclose(f);
    free(buffer);
    return 1;
}

int download(int* cfd, const char* command) {

    if (write_with_check_str(cfd, command, strlen(command) + 1, NULL) == -2)
        return 0;

    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    long fileSize = -1, received = 0;
    int rec;

    char* filePath = get_file_path(settings->file_path, get_filename(command + 9));
    FILE* f = fopen(filePath, "wb");
    if (f == NULL)
        return -1;

    if (read_with_check_long(cfd, &fileSize, f) == -2)
        return 0;
    if (read_with_check_long(cfd, &received, f) == -2)
        return 0;
    if (received != 0)
        fseek(f, received, SEEK_SET);

    double recTime = 0.0, packTime = 0.0;
    struct timeval start, end;
    LLINE* lline = init_lline(((double)received / fileSize) * 100.0, fileSize);
    show_lline(lline);

    gettimeofday(&start, NULL);
    while (received < fileSize && (rec = read_with_check_str(cfd, &buffer, BUFFER_SIZE, f))) {
        if (rec == -2)
            return 0;
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
    printf("\rSuccessfully receive data from server. Speed: "GREEN"%s"RESET"; Time: "CYAN"%.2fs"RESET"\n", 
                                        get_speed_stirng(get_speed(fileSize, 100.0, recTime)), recTime);
    fclose(f);
    free(filePath);
    free(buffer);
    return 1;
}