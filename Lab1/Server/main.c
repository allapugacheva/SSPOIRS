#include "server.h"

int main(int argc, char* argv[]) {

    int port = argc == 2 ? atoi(argv[1]) : 8080;
    run(port);
    return 0;
}