#include "client.h"

int main(int argc, char* argv[]) {

    setlocale(LC_ALL, "");
    if (argc < 2) {
        perror("Server address not specified.\n");
        exit(-1);
    }
    int port = argc == 3 ? atoi(argv[2]) : 8080;

    run(argv[1], port);
    return 0;
}