#ifndef CHAT_BACKBONE_H_
#define CHAT_BACKBONE_H_

#include <sys/types.h>
#include <list>
#include "commons.h"

struct user;
typedef struct user * User;

class ChatBackbone {
    protected:
        int sockfd, portno;
        fd_set read_fds; 
        std::list<User> users;
        int fdmax; 

    public:
        ChatBackbone();
        ~ChatBackbone();

        void connectToServer(int &sockfd, const struct sockaddr_in &serv_addr);
        void addReadFD(int fd);
        User findUser(const char *name);
        void printList();
        void error(const char *msg);
        void trim_endl(char *string, int len);
};

#endif /*CHAT_BACKBONE_H_*/
