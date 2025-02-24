#include "check_connection.h"

void setup_keepalive(SCKT* sockfd) {

    int optval = 1;

    #if _WIN32
    setsockopt(*sockfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, sizeof(optval));

    struct tcp_keepalive ka;
    ka.onoff = 1;           
    ka.keepalivetime = 10000;    
    ka.keepaliveinterval = 2000; 

    DWORD bytesReturned;
    WSAIoctl(*sockfd, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), NULL, 0, &bytesReturned, NULL, NULL);
    #elif __linux__
    setsockopt(*sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));

    int idle = 10;     
    int interval = 2;  
    int maxpkt = 3;   

    setsockopt(*sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(*sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(*sockfd, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(maxpkt));
    #endif
}

int check_connection(SCKT* sockfd) {

    char buffer[1];
    int ret;
    if ((ret = recv(*sockfd, buffer, 1, MSG_PEEK)) <= 0) {
        #ifdef _WIN32
        int err = WSAGetLastError();
        if (ret == 0 || (ret == -1 && (err == WSAECONNRESET || err == WSAESHUTDOWN)))
        #elif __linux__
        if (ret == 0 || (ret == 1 && (errno == ECONNRESET || errno == EPIPE))) 
        #endif
        {
            close(*sockfd);
            *sockfd = -1;
            return 0;
        }
    }
    return 1;
}

int write_with_check_str(SCKT* sockfd, const char* buffer, int len, FILE* f) {

    #ifdef _WIN32
    return process_result(write(*sockfd, buffer, len), sockfd, f);
    #elif __linux__
    return process_result(send(*sockfd, buffer, len, 0), sockfd, f);
    #endif
}

int read_with_check_str(SCKT* sockfd, char** buffer, int len, FILE* f) {
    
    #ifdef _WIN32
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(*sockfd, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int ready = select(*sockfd + 1, &readfds, NULL, NULL, &timeout);

    if (ready == SOCKET_ERROR || ready == 0) 
        return -1;

    if (FD_ISSET(*sockfd, &readfds)) {
        int ret = read(*sockfd, *buffer, len);
        return process_result(ret, sockfd, f);
    }
    return 0;
    #elif __linux__ 
    return process_result(recv(*sockfd, *buffer, len, 0), sockfd, f);
    #endif
}

int write_with_check_wstr(SCKT* sockfd, const wchar_t* buffer, int len, FILE* f) {

    #ifdef _WIN32
    size_t utf8_len = len * sizeof(wchar_t) + 1;
    char* utf8_buffer = (char*)malloc(utf8_len);
    wcstombs(utf8_buffer, buffer, utf8_len);

    return write_with_check_str(sockfd, utf8_buffer, utf8_len, f);
    #elif __linux__
    return process_result(send(*sockfd, buffer, len * sizeof(wchar_t), 0), sockfd, f);
    #endif
}

int read_with_check_wstr(SCKT* sockfd, wchar_t** buffer, int len, FILE* f) {

    #ifdef _WIN32
    int ret;
    char* utf8_buffer = (char*)malloc(len * sizeof(wchar_t) + 1);
    if ((ret = read_with_check_str(sockfd, &utf8_buffer, len, f)) <= 0) {
        free(utf8_buffer);
        return ret;
    }

    size_t unicode_len = strlen(utf8_buffer) + 1;
    mbstowcs(*buffer, utf8_buffer, unicode_len);
    free(utf8_buffer);

    return ret;
    #elif __linux__
    return process_result(read(*sockfd, *buffer, len * sizeof(wchar_t)), sockfd, f);
    #endif
}

int write_with_check_long(SCKT* sockfd, long* buffer, FILE* f) {

    #ifdef _WIN32
    char temp[sizeof(long) + 1];
    sprintf(temp, "%ld", *buffer);
    temp[sizeof(long)] = '\0';

    return write_with_check_str(sockfd, temp, sizeof(long) + 1, f);
    #elif __linux__
    return process_result(write(*sockfd, buffer, sizeof(*buffer)), sockfd, f);
    #endif
}

int read_with_check_long(SCKT* sockfd, long* buffer, FILE* f) {

    #ifdef _WIN32
    int res;
    char* temp = (char*)malloc(sizeof(long));
    if ((res= read_with_check_str(sockfd, &temp, sizeof(long), f)) <= 0) {
        free(temp);
        return res;
    }
    *buffer = atol(temp);

    return res;
    #elif __linux__
    return process_result(read(*sockfd, buffer, sizeof(*buffer)), sockfd, f);
    #endif
}

int process_result(int ret, SCKT* sockfd, FILE* f) {

    #ifdef _WIN32
    if (ret == 0 || (ret == -1 && (WSAGetLastError() == WSAECONNRESET)))
    #elif __linux__
    if (ret == 0 || (ret == -1 && errno == ECONNRESET))
    #endif 
    {
        if (f != NULL)
            fclose(f);
        wprintf(L"\rСоединение разорвано.\n> ");    
        close(*sockfd);
        *sockfd = INVLD_SCKT;
        return -2;
    }
    return ret;
}