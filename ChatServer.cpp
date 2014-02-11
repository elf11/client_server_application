#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <time.h>
#include <list>
#include "ChatServer.h"

using namespace std;


ChatServer::ChatServer(int portno) {
    this->portno = portno;

    setupServer();
}


void ChatServer::setupServer() {
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;   
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0)
        error("ERROR on binding");

    listen(sockfd, MAX_CLIENTS);
}

void ChatServer::run() {
    fd_set tmp_fds; 
    User crt_user;
    char buffer[256];
    int read_bytes;
    time_t tstart, tend;

    addReadFD(STDIN_FILENO);
    addReadFD(sockfd);

    printf("server> ");
    fflush(stdout);

    tstart = time(NULL);

    for(;;) {
        tend = time(NULL);

        if(tend - tstart >= 5) {
            #ifdef DEBUG
                printf("\ndebug: the time has come to ping everybody\n");
                printf("debug: time_start=%ld time_end=%ld\n", tstart, tend);
                printf("debug> ");
                fflush(stdout);
            #endif

            checkPingedOnes();

            pingEverybody();

            initPingedOnes();

            tstart = tend;
        }

        tmp_fds = read_fds;

        if(select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
            error("ERROR in select");

        if(FD_ISSET(STDIN_FILENO, &tmp_fds)) {
            memset(buffer, 0, 256);
            read_bytes = read(STDIN_FILENO, buffer, 255);

            if(read_bytes < 0)
                error("ERROR in stdin");

            trim_endl(buffer, read_bytes);

            if(strcmp(buffer, "list") == 0) {
                printList();
            } else if(strcmp(buffer, "quit") == 0) {
                printf("The ChatServer is exiting...\n");
                break;
            }

            printf("server> ");
            fflush(stdout);
        }

        if(FD_ISSET(sockfd, &tmp_fds))
            handleNewConnection();

        list<User>::iterator iter = users.begin();


        while(iter != users.end()) {
            bool deletion = false;

            crt_user = *iter;

            int usr_sockfd = crt_user->sockfd;

            if(FD_ISSET(usr_sockfd, &tmp_fds))
                deletion = handleClientMessage(crt_user, usr_sockfd);

            if(deletion) {
                iter = users.erase(iter);
            } else {
                ++iter;
            }
        }
    }
}

void ChatServer::checkPingedOnes() {
    map<struct user_info, bool, less_info>::iterator map_iter;
    User crt_user;

    list<User>::iterator iter = users.begin();

    while(iter != users.end()) {
        crt_user = *iter;

        if(crt_user->auth) {
            map_iter = reply_status.find(crt_user->info);

            if(map_iter != reply_status.end() && !(map_iter->second)) {

                printf("\ndel user: %s\n", crt_user->info.name);
                printf("server> ");
                fflush(stdout);

                crt_user->auth = false;
                updateAllOnRemove(crt_user, crt_user->sockfd);

                close(crt_user->sockfd);
                FD_CLR(crt_user->sockfd, &read_fds);
                free(crt_user);

                iter = users.erase(iter);
            } else {
                ++iter;
            }
        } else {
            ++iter;
        }
    }
}


void ChatServer::pingEverybody() {
    struct list_update_packet packet;
    User crt_user;
    int bytes_written;

    memset((char *) &packet, 0, sizeof(struct list_update_packet));
    packet.type = PING;

    for(list<User>::iterator iter = users.begin(); iter != users.end(); ++iter) {
        crt_user = *iter;

        if(crt_user->auth) {
            int usr_sockfd = crt_user->sockfd;

            bytes_written = send(usr_sockfd, &packet, sizeof(struct list_update_packet), 0);
            if (bytes_written < 0)
                error("ERROR writing to socket");
        }
    }
}


void ChatServer::initPingedOnes() {
    User crt_user;

    reply_status.clear();

    for(list<User>::iterator iter = users.begin(); iter != users.end(); ++iter) {
        crt_user = *iter;

        if(crt_user->auth)
            reply_status[crt_user->info] = false;
    }
}

void ChatServer::handleNewConnection() {
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    int newsockfd;
    User new_user;

    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd == -1) { 
        error("ERROR in accept");
    } else { 
        addReadFD(newsockfd);

        new_user = (User) calloc(1, sizeof(struct user));

        new_user->sockfd = newsockfd;
        new_user->auth = false;
        new_user->info.address = cli_addr.sin_addr;

        users.push_front(new_user);
    }
}

bool ChatServer::handleClientMessage(User crt_user, int usr_sockfd) {
    bool deletion;

    deletion = crt_user->auth ?
        handleClientPing(crt_user, usr_sockfd) :
        handleClientAuth(crt_user, usr_sockfd);

    if(deletion) {
        if(crt_user->auth) {
            printf("\ndel user: %s\n", crt_user->info.name);
            printf("server> ");
            fflush(stdout);

            crt_user->auth = false;
            updateAllOnRemove(crt_user, usr_sockfd);

        }

        close(usr_sockfd);
        FD_CLR(usr_sockfd, &read_fds);
        free(crt_user);
    }

    return deletion;
}

bool ChatServer::handleClientPing(User crt_user, int usr_sockfd) {
    char buffer[256];
    int read_bytes;

    memset(buffer, 0, 256);
    read_bytes = recv(usr_sockfd, buffer, 3, 0);

    if (read_bytes < 0)
        error("ERROR reading from socket");

    if (read_bytes == 0)
        return true;

    if(strcmp(buffer, "ACK") != 0)
        return true;

    reply_status[crt_user->info] = true;

    #ifdef DEBUG
        printf("\ndebug: we have received an ACK to the ping given to %s\n", crt_user->info.name);
        printf("debug> ");
        fflush(stdout);
    #endif

    return false;
}

bool ChatServer::handleClientAuth(User crt_user, int usr_sockfd) {
    struct auth_packet packet;
    char buffer[256];
    int read_bytes, written_bytes;

    memset((char *) &packet, 0, sizeof(struct auth_packet));

    read_bytes = recv(usr_sockfd, &packet, sizeof(struct auth_packet), 0);

    if(read_bytes < 0)
        error("ERROR in recv");

    if(read_bytes == 0)
        return true;

    if(findUser(packet.name) == NULL) {
        strcpy(crt_user->info.name, packet.name);
        crt_user->info.port = packet.port;
        crt_user->info.file_port = packet.file_port;

        memset(buffer, 0 , 256);
        strcpy(buffer, "ACK");
        written_bytes = send(usr_sockfd, buffer, strlen(buffer), 0);

        if (written_bytes < 0)
            error("ERROR writing to socket");

        updateAllOnAdd(crt_user, usr_sockfd);

        crt_user->auth = true;

        struct user_info * crt_info = & crt_user->info;
        printf("\nadd user: %s %s %d\n", crt_info->name, inet_ntoa(crt_info->address), crt_info->port);
        printf("server> ");
        fflush(stdout);

        return false;
    } else {
        memset(buffer, 0 , 256);
        strcpy(buffer, "NAK");
        written_bytes = send(usr_sockfd, buffer, strlen(buffer), 0);
        if (written_bytes < 0)
             error("ERROR writing to socket");

        return true;
    }
}


void ChatServer::updateAllOnAdd(User new_user, int newsockfd) {
    struct list_update_packet packet, newusr_packet;
    list<User>::iterator iter;
    User crt_user;
    int written_bytes;

    memset((char *) &newusr_packet, 0, sizeof(struct list_update_packet));
    newusr_packet.type = CONNECTED;
    newusr_packet.info = new_user->info;

    for(iter = users.begin(); iter != users.end(); ++iter) {
        crt_user = *iter;

        if(crt_user->auth) {
            written_bytes = send(crt_user->sockfd, &newusr_packet, sizeof(struct list_update_packet), 0);
            if (written_bytes < 0)
                error("ERROR writing to socket");

            memset((char *) &packet, 0, sizeof(struct list_update_packet));
            packet.type = CONNECTED;
            packet.info = crt_user->info;

            written_bytes = send(newsockfd, &packet, sizeof(struct list_update_packet), 0);
            if (written_bytes < 0)
                error("ERROR writing to socket");
        }
    }
}


void ChatServer::updateAllOnRemove(User new_user, int newsockfd) {
    struct list_update_packet oldusr_packet;
    list<User>::iterator iter;
    User crt_user;
    int written_bytes;

    memset((char *) &oldusr_packet, 0, sizeof(struct list_update_packet));
    oldusr_packet.type = DISCONNECTED;
    oldusr_packet.info = new_user->info;

    for(iter = users.begin(); iter != users.end(); ++iter) {
        crt_user = *iter;

        if(crt_user->auth) {
            written_bytes = send(crt_user->sockfd, &oldusr_packet, sizeof(struct list_update_packet), 0);
            if (written_bytes < 0)
                 error("ERROR writing to socket");
        }
    }
}
