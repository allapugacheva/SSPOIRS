#include "check_connection.h"

void setup_keepalive(int* sockfd) {

    int optval = 1;
    if (setsockopt(*sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == -1) {
        perror("Error set keepalive option");
        close(*sockfd);
        exit(errno);
    }

    int idle = 10;     
    int interval = 2;  
    int maxpkt = 3;   

    setsockopt(*sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(*sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(*sockfd, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(maxpkt));
}

int check_connection(int* sockfd, FILE* logger) {

    int ret = send(*sockfd, NULL, 0, MSG_NOSIGNAL);
    if (ret == -1) {
        if (errno == ECONNRESET || errno == EPIPE) {
            printf("Lost connection\n");
            log_message(logger, LOG_ERROR, "Lost connection");
            close(*sockfd);
            *sockfd = -1;
            return 0;
        }
    }
    return 1;
}

int write_with_check(int* sockfd, const char* buffer, int len, FILE* logger, FILE* f) {

    int ret = write(*sockfd, buffer, len);
    if (ret == -1 && (errno == ECONNRESET || errno == EPIPE)) {
        if(f != NULL)
            fclose(f);
        printf("Lost connection\n");
        log_message(logger, LOG_ERROR, "Lost connection");
        close(*sockfd);
        *sockfd = -1;
        return -2;
    }
    return ret;
}

int read_with_check(int* sockfd, char** buffer, int len, FILE* logger, FILE* f) {

    int ret = read(*sockfd, *buffer, len);
    if (ret == 0 || (ret == -1 && errno == ECONNRESET)) {
        if (f != NULL)
            fclose(f);
        printf("Lost connection\n");
        log_message(logger, LOG_ERROR, "Lost connection");
        close(*sockfd);
        *sockfd = -1;
        return -2;
    }
    return ret;
}

int write_with_check_int(int* sockfd, long double* buffer, FILE* logger, FILE* f) {

    int ret = write(*sockfd, buffer, sizeof(long double));
    if (ret == -1 && (errno == ECONNRESET || errno == EPIPE)) {
        if (f != NULL)
            fclose(f);
        printf("Lost connection\n");    
        log_message(logger, LOG_ERROR, "Lost connection");
        close(*sockfd);
        *sockfd = -1;
        return -2;
    }
    return ret;
}

int read_with_check_int(int* sockfd, long double* buffer, FILE* logger, FILE* f) {

    int ret = read(*sockfd, buffer, sizeof(long double));
    if (ret == 0 || (ret == -1 && errno == ECONNRESET)) {
        if (f != NULL)
            fclose(f);
        printf("Lost connection\n");    
        log_message(logger, LOG_ERROR, "Lost connection");
        close(*sockfd);
        *sockfd = -1;
        return -2;
    }
    return ret;
}