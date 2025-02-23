#include "server.h"

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
    int port = argc == 2 ? atoi(argv[1]) : 8080;
    run(port);
    return 0;
}