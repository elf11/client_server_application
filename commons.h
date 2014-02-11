#ifndef COMMONS_H_
#define COMMONS_H_

#include <netinet/in.h>
#include <cstring>

#define MAX_CLIENTS    5
#define NAME_LENGTH 30
#define BUFFER_SIZE 256
#define CONNECTED 0xeea
#define DISCONNECTED 0xfff
#define PING 0xabc


struct user_info {
    char name[NAME_LENGTH];
    struct in_addr address;
    int port;
    int file_port;
};

struct user {
    int sockfd;
    bool auth;
    struct user_info info;
};

struct auth_packet {
    char name[NAME_LENGTH];
    int port;
    int file_port;
};

struct list_update_packet {
    short type;
    struct user_info info;
};

struct less_info {
    bool operator() (const struct user_info &lhs, const struct user_info &rhs) const {
        if(lhs.port < rhs.port)
            return true;
        else if(lhs.port > rhs.port)
            return false;
        else if(lhs.address.s_addr < rhs.address.s_addr)
            return true;
        else if(lhs.address.s_addr > rhs.address.s_addr)
            return false;
        else if(strcmp(lhs.name, rhs.name) < 0)
            return true;
        else
            return false;
    }
};

struct equal_info {
    bool operator() (const struct user_info &lhs, const struct user_info &rhs) const {
        return (lhs.port == rhs.port) &&
            (lhs.address.s_addr == rhs.address.s_addr) &&
            (strcmp(lhs.name, rhs.name) == 0);
    }
};

#endif /*COMMONS_H_*/
