#include "client.h"

int main(int argc, char* argv[]) {

    #ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    #endif

    setlocale(LC_ALL, "");
    if (argc < 2) {
        perror("Server address not specified.\n");
        exit(-1);
    }
    int port = argc == 3 ? atoi(argv[2]) : 8080;

    run(argv[1], port);
    return 0;
}