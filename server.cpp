#include <cstdio>
#include <cstdlib>
#include "ChatServer.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,"Usage : %s port\n", argv[0]);
        exit(1);
    }

    ChatServer server(atoi(argv[1]));

    server.run();

    return 0;
}
