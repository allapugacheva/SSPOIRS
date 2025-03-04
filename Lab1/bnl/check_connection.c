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
        if (ret == 0 || (ret == -1 && (errno == ECONNRESET || errno == EPIPE))) 
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
    return process_result(send(*sockfd, buffer, len, 0), sockfd, f);
    #elif __linux__
    return process_result(send(*sockfd, buffer, len, MSG_DONTWAIT), sockfd, f);
    #endif
}

int read_with_check_str(SCKT* sockfd, char** buffer, int len, FILE* f) {
    
    #ifdef _WIN32
    return process_result(recv(*sockfd, *buffer, len, 0), sockfd, f);
    #elif __linux
    return process_result(recv(*sockfd, *buffer, len, MSG_DONTWAIT), sockfd, f);
    #endif
}

int write_with_check_wstr(SCKT* sockfd, const wchar_t* buffer, int len, FILE* f) {

    #ifdef _WIN32
    return process_result(send(*sockfd, (char*)buffer, len * sizeof(wchar_t), 0), sockfd, f);
    #elif __linux__
    return process_result(send(*sockfd, buffer, len * sizeof(wchar_t), MSG_DONTWAIT), sockfd, f);
    #endif
}

int read_with_check_wstr(SCKT* sockfd, wchar_t** buffer, int len, FILE* f) {

    #ifdef _WIN32
    return process_result(recv(*sockfd, (char*)*buffer, len * sizeof(wchar_t), 0), sockfd, f);
    #elif __linux__
    return process_result(recv(*sockfd, *buffer, len * sizeof(wchar_t), MSG_DONTWAIT), sockfd, f);
    #endif
}

int write_with_check_long(SCKT* sockfd, long* buffer, FILE* f) {

    #ifdef _WIN32
    return process_result(send(*sockfd, (char*)buffer, sizeof(*buffer), 0), sockfd, f);
    #elif
    return process_result(send(*sockfd, buffer, sizeof(*buffer), MSG_DONTWAIT), sockfd, f);
    #endif
}

int read_with_check_long(SCKT* sockfd, long* buffer, FILE* f) {

    #ifdef _WIN32
    return process_result(recv(*sockfd, (char*)buffer, sizeof(*buffer), 0), sockfd, f);
    #elif
    return process_result(recv(*sockfd, buffer, sizeof(*buffer), MSG_DONTWAIT), sockfd, f);
    #endif
}

int process_result(int ret, SCKT* sockfd, FILE* f) {

    #ifdef _WIN32
    if (ret == -1 && (WSAGetLastError() == WSAECONNRESET))
    #elif __linux__
    if (ret == -1 && errno == ECONNRESET)
    #endif 
    {
        if (f != NULL)
            fclose(f);
        wprintf(L"\rСоединение разорвано.             \n> ");    
        close(*sockfd);
        *sockfd = INVLD_SCKT;
        return ret;
    }
    #ifdef _WIN32
    if (ret == -1 && WSAGetLastError() == WSAEWOULDBLOCK)
    #elif __linux__
    if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
    #endif
        return 0; 
    return ret;
}