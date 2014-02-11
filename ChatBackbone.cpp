#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>
#include "ChatBackbone.h"

using namespace std;


ChatBackbone::ChatBackbone() {
    FD_ZERO(&read_fds);
    fdmax = 0;
}


ChatBackbone::~ChatBackbone() {
    list<User>::iterator iter;
    User crt_user;

    for(iter = users.begin(); iter != users.end(); ++iter) {
        crt_user = *iter;
        close(crt_user->sockfd);
        free(crt_user);
    }

    close(sockfd);
}

void ChatBackbone::connectToServer(int &sockfd, const struct sockaddr_in &serv_addr) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in)) < 0)
        error("ERROR connecting");
}


void ChatBackbone::addReadFD(int fd) {

    FD_SET(fd, &read_fds);

    if (fd > fdmax) {
        fdmax = fd;
    }
}

User ChatBackbone::findUser(const char *name) {
    list<User>::iterator iter;
    User crt_user;

    for(iter = users.begin(); iter != users.end(); ++iter) {
        crt_user = *iter;

        if(crt_user->auth && strcmp(name, crt_user->info.name) == 0)
            return crt_user;
    }

    return NULL;
}

void ChatBackbone::printList() {
    list<User>::iterator iter;
    User crt_user;

    for(iter = users.begin(); iter != users.end(); ++iter) {
        crt_user = *iter;

        if(crt_user->auth) {
            struct user_info * crt_info = & crt_user->info;
            printf("%s %s %d\n", crt_info->name, inet_ntoa(crt_info->address), crt_info->port);
        }
    }
}

void ChatBackbone::error(const char *msg) {
    perror(msg);
    exit(1);
}

void ChatBackbone::trim_endl(char *string, int len) {
    do {
        --len;
    } while((len >= 0) && (string[len] == '\n' || string[len] == '\r'));

    string[len + 1] = 0;
}

