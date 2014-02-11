#ifndef CHATCLIENT_H_
#define CHATCLIENT_H_

#include <sys/types.h>
#include <list>
#include <string>
#include "ChatBackbone.h"
#include "MessageArchive.h"


struct user;
typedef struct user * User;

class ChatClient : public ChatBackbone {
    private:
        int serv_sockfd, serv_portno;
        int file_sockfd, file_portno;
        char *name, *down_dir;
        std::vector < std::string > files; 
        struct hostent * server;
        MessageArchive archive;

        bool send_mode;
        bool file_mode;

        bool exit_now;

        void setupServer();
        void connectToChatServer();

    public:
        ChatClient(char *name, char *server_address, char *server_port);
        ChatClient(char *name, char *server_address, char *server_port, char *down_dir);
        ~ChatClient();

        void run();
        void handleInputFromUser();
        void sendMessageToClient(User receiver, char buffer[]);
        void handleMessageFromServer();
        void handleNewConnection();
        void sendFile(User crt_user, char *filename);
        void receiveFile();
        void sendAuthPacket(int auth_sockfd);
};

#endif /*CHATCLIENT_H_*/
