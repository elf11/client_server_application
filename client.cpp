#include <cstdio>
#include <cstdlib>
#include "ChatClient.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr,"Usage %s username hostname port\n", argv[0]);
        exit(0);
    }

    if(argc > 4) {
        ChatClient client(argv[1], argv[2], argv[3], argv[4]);
        client.run();
    } else {
        ChatClient client(argv[1], argv[2], argv[3]);
        client.run();
    }

    return 0;
}

