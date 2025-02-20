#include "check_connection.h"

void setup_keepalive(int* sockfd) {

    int optval = 1;
    setsockopt(*sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));

    int idle = 10;     
    int interval = 2;  
    int maxpkt = 3;   

    setsockopt(*sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(*sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(*sockfd, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(maxpkt));
}

int check_connection(int* sockfd) {

    char buffer[1];
    int ret;
    if ((ret = recv(*sockfd, buffer, 1, MSG_PEEK)) <= 0) {
        if (ret == 0 || (ret == 1 && (errno == ECONNRESET || errno == EPIPE))) {
            close(*sockfd);
            *sockfd = -1;
            return 0;
        }
    }
    return 1;
}

int write_with_check_str(int* sockfd, const char* buffer, int len, FILE* f) {

    return process_result(write(*sockfd, buffer, len), sockfd, f);
}

int read_with_check_str(int* sockfd, char** buffer, int len, FILE* f) {

    return process_result(read(*sockfd, *buffer, len), sockfd, f);
}

int write_with_check_long(int* sockfd, long* buffer, FILE* f) {

    return process_result(write(*sockfd, buffer, sizeof(long)), sockfd, f);
}

int read_with_check_long(int* sockfd, long* buffer, FILE* f) {

    return process_result(read(*sockfd, buffer, sizeof(long)), sockfd, f);
}

int process_result (int ret, int* sockfd, FILE* f) {

    if (ret == 0 || (ret == -1 && errno == ECONNRESET)) {
        if (f != NULL)
            fclose(f);
        printf("\rLost connection.\n");    
        close(*sockfd);
        *sockfd = -1;
        return -2;
    }
    return ret;
}