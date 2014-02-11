#ifndef CHATSERVER_H_
#define CHATSERVER_H_

#include <sys/types.h>
#include <list>
#include <map>
#include "ChatBackbone.h"

struct user;
typedef struct user * User;

class ChatServer : public ChatBackbone {
    private:
        std::map<struct user_info, bool, less_info> reply_status;

    public:
        ChatServer(int portno);
        ~ChatServer() {}

        void setupServer();
        void run();
        void checkPingedOnes();
        void pingEverybody();
        void initPingedOnes();
        void handleNewConnection();
        bool handleClientMessage(User crt_user, int usr_sockfd);
        bool handleClientPing(User crt_user, int usr_sockfd);
        bool handleClientAuth(User crt_user, int usr_sockfd);
        void updateAllOnAdd(User new_user, int newsockfd);
        void updateAllOnRemove(User new_user, int newsockfd);
};

#endif /*CHATSERVER_H_*/
