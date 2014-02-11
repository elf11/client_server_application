#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "ChatClient.h"

extern "C" {
    #include "kermit.h"
}

using namespace std;

ChatClient::ChatClient(char *name, char *server_address, char *server_port) : name(name) {
    server = gethostbyname(server_address);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    serv_portno = atoi(server_port);

    send_mode = false;
    file_mode = false;

    exit_now = false;

    setupServer();
}

ChatClient::ChatClient(char *name, char *server_address, char *server_port, char *down_dir) : name(name), down_dir(down_dir) {
    DIR *my_dir = opendir(down_dir);
    if(my_dir == NULL) {
        printf("Folderul %s nu exista.\n", down_dir);
        exit(-1);
    } else {
        closedir(my_dir);
    }

    server = gethostbyname(server_address);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    serv_portno = atoi(server_port);

    send_mode = false;
    file_mode = true;

    exit_now = false;

    setupServer();
}

void ChatClient::setupServer() {
    struct sockaddr_in serv_addr;
    socklen_t servlen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = 0;

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0)
        error("binding stream socket");

    servlen = sizeof(serv_addr);
    if (getsockname(sockfd, (struct sockaddr *) &serv_addr, &servlen))
        error("getting socket name");

    portno = ntohs(serv_addr.sin_port);

    listen(sockfd, MAX_CLIENTS);

    if(file_mode) {
        file_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (file_sockfd < 0)
            error("ERROR opening socket");

        memset((char *) &serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = 0;

        if (bind(file_sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0)
            error("binding stream socket");

        servlen = sizeof(serv_addr);
        if (getsockname(file_sockfd, (struct sockaddr *) &serv_addr, &servlen))
            error("getting socket name");

        file_portno = ntohs(serv_addr.sin_port);

        listen(file_sockfd, MAX_CLIENTS);
    } else { 
        file_portno = portno; 
    }
}

ChatClient::~ChatClient() {
    close(serv_sockfd);
    if(file_mode)
        close(file_sockfd);
}

void ChatClient::run() {
    char buffer[BUFFER_SIZE];
    int read_bytes;
    User crt_user;
    int usr_sockfd;
    fd_set tmp_fds; 

    connectToChatServer();

    sendAuthPacket(serv_sockfd);

    memset(buffer, 0, BUFFER_SIZE);
    read_bytes = recv(serv_sockfd, buffer, 3, 0);

    if (read_bytes < 0)
        error("ERROR reading from socket");

    if (read_bytes == 0) {
        printf("S-a intrerupt conexiunea cu serverul.\n");
        return;
    }

    if (strcmp(buffer, "ACK") != 0) {
        printf("Autentificarea nu a reusit... exista deja client cu numele \"%s\"\n", name);
        exit(-1);
    }

    addReadFD(STDIN_FILENO);
    addReadFD(serv_sockfd);
    addReadFD(sockfd);
    if(file_mode)
        addReadFD(file_sockfd);

    printf("client> ");
    fflush(stdout);

    for(;;) {
        tmp_fds = read_fds;

        if(select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
            error("ERROR in select");

        if(FD_ISSET(STDIN_FILENO, &tmp_fds)) {
            handleInputFromUser();

            if(exit_now)
                return;
        }

        if(FD_ISSET(serv_sockfd, &tmp_fds)) {
            handleMessageFromServer();

            if(exit_now) {
                printf("\nS-a intrerupt conexiunea cu serverul.\n");
                return;
            }
        }

        if(FD_ISSET(sockfd, &tmp_fds))
            handleNewConnection();

        if(file_mode && FD_ISSET(file_sockfd, &tmp_fds))
            receiveFile();

        list<User>::iterator iter = users.begin();

        while(iter != users.end()) {
            bool deletion = false;

            crt_user = *iter;
            usr_sockfd = crt_user->sockfd;

            if(usr_sockfd != sockfd && FD_ISSET(usr_sockfd, &tmp_fds)) {
                memset(buffer, 0, BUFFER_SIZE);

                read_bytes = recv(usr_sockfd, buffer, BUFFER_SIZE, 0);
                if (read_bytes < 0)
                    error("ERROR reading from socket");

                if(read_bytes == 0) {
                    deletion = true;
                } else {
                    if(archive.addMessage(crt_user->info, buffer) && !send_mode) {
                        printf("client> ");
                        fflush(stdout);
                    }
                }
            }

            if(deletion) {
                close(usr_sockfd);
                FD_CLR(usr_sockfd, &read_fds);
                archive.resetMessagesFrom(crt_user->info);
                free(crt_user);
                iter = users.erase(iter);
            } else {
                ++iter;
            }
        }
    }
}

void ChatClient::connectToChatServer() {
    struct sockaddr_in serv_addr;
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *) &serv_addr.sin_addr.s_addr,
           (char *) server->h_addr,
           server->h_length);
    serv_addr.sin_port = htons(serv_portno);

    connectToServer(serv_sockfd, serv_addr);
}

void ChatClient::handleInputFromUser() {
    static User receiver = NULL;
    static string receiver_name;
    char buffer[BUFFER_SIZE];
    int read_bytes;

    memset(buffer, 0, BUFFER_SIZE);
    read_bytes = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);

    if(read_bytes < 0)
        error("ERROR in stdin");

    trim_endl(buffer, read_bytes);

    if(!send_mode) {
        if(strcmp(buffer, "list") == 0) {
            printList();
        } else if(strncmp(buffer, "send", 4) == 0) {
            char *p;

            p = strtok(buffer, " ");
            p = strtok(NULL, " ");

            if(p != NULL) {
                receiver = findUser(p);

                if(receiver != NULL) {
                    receiver_name = string(p);
                    send_mode = true;
                } else {
                    printf("Userul %s nu se afla in lista.\n", p);
                }
            } else {
                printf("Comanda invalida. Formatul corect este: send [user].\n");
            }
        } else if(strncmp(buffer, "read", 4) == 0) { 
            char *p;

            p = strtok(buffer, " ");
            p = strtok(NULL, " ");

            if(p != NULL) {
                User crt_user = findUser(p);

                if(crt_user != NULL) {
                    char *q = strtok(NULL, " ");

                    if(q != NULL) {
                        archive.printMessageFromNo(crt_user->info, atoi(q));
                    } else {
                        archive.printAvailableMessagesFrom(crt_user->info);
                    }
                } else {
                    printf("Userul %s nu se afla in lista.\n", p);
                }
            } else {
                archive.printAllAvailableMessages();
            }
        } else if(strncmp(buffer, "fsend", 5) == 0) { 
            char *p;

            p = strtok(buffer, " ");
            p = strtok(NULL, " ");

            if(p != NULL) {
                User crt_user = findUser(p);

                if(crt_user != NULL) {
                    char *q = strtok(NULL, " ");

                    if(q != NULL) { // transmit fisierul
                        if(crt_user->info.file_port != crt_user->info.port)
                            sendFile(crt_user, q);
                        else
                            printf("Utilizatorul %s nu stie sa primeasca fisiere.\n", p);
                    } else {
                        printf("Trebuie sa dati si numele fisierului ce va fi transmis lui %s.\n", p);
                    }
                } else {
                    printf("Userul %s nu se afla in lista.\n", p);
                }
            } else {
                printf("Formatul comenzii este: fsend [nume_utilizator] [cale_fisier]\n");
            }
        } else if(file_mode && strcmp(buffer, "flist") == 0) {
            if(files.size() == 0) {
                printf("Nu a fost primit nici un fisier.\n");
            } else {
                printf("Fisierele primite in folderul %s de la diversi utilizatori:\n", down_dir);
                // le afisez
                for(unsigned int i = 0; i < files.size(); i++)
                    printf("%s\n", files[i].c_str());
            }
        } else if(strcmp(buffer, "quit") == 0) {
            printf("The ChatClient is exiting...\n");
            exit_now = true;
            return;
        }

        if(!send_mode) {
            printf("client> ");
            fflush(stdout);
        }
    } else { 
        User alleged_receiver = findUser(receiver_name.c_str());

        if(alleged_receiver != NULL && equal_info() (receiver->info, alleged_receiver->info)) {
            sendMessageToClient(receiver, buffer);
        } else { 
            send_mode = false;

            // mini-prompt
            printf("client> ");
            fflush(stdout);
        }
    }
}


void ChatClient::sendMessageToClient(User receiver, char buffer[]) {
    int written_bytes;

    if(receiver->sockfd == sockfd) {
        int newsockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (newsockfd < 0)
            error("ERROR opening socket");

        struct sockaddr_in serv_addr;
        memset((char *) &serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr = receiver->info.address;
        serv_addr.sin_port = htons(receiver->info.port);

        if (connect(newsockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
            error("ERROR connecting");

        receiver->sockfd = newsockfd;

        addReadFD(newsockfd);

        sendAuthPacket(newsockfd);
    }

    written_bytes = send(receiver->sockfd, buffer, BUFFER_SIZE, 0);
    if (written_bytes < 0)
        error("ERROR writing to socket");

    if(strcmp(buffer, ".") == 0) {
        send_mode = false;

        printf("client> ");
        fflush(stdout);
    }
}


void ChatClient::handleMessageFromServer() {
    struct list_update_packet packet;
    char buffer[BUFFER_SIZE];
    int read_bytes, written_bytes;
    User new_user;

    read_bytes = recv(serv_sockfd, &packet, sizeof(struct list_update_packet), 0);

    if (read_bytes < 0)
        error("ERROR reading from socket");

    if(read_bytes == 0) {
        exit_now = true;
        return;
    }

    if(packet.type == CONNECTED) { 
        new_user = (User) malloc(sizeof(struct user));

        new_user->sockfd = sockfd;
        
        new_user->auth = true;
        new_user->info = packet.info;

        users.push_front(new_user);
    } else if(packet.type == DISCONNECTED) { 
        list<User>::iterator iter = users.begin();

        while(iter != users.end()) {
            User crt_user = *iter;

            int usr_sockfd = crt_user->sockfd;

            if(equal_info() (crt_user->info, packet.info)) {
                if(usr_sockfd != sockfd) {
                    close(usr_sockfd);
                    FD_CLR(usr_sockfd, &read_fds);
                    archive.resetMessagesFrom(crt_user->info);
                }

                free(crt_user);
                users.erase(iter);

                break;
            }

            ++iter;
        }
    } else if(packet.type == PING) { 
        memset(buffer, 0 , BUFFER_SIZE);
        strcpy(buffer, "ACK");
        written_bytes = send(serv_sockfd, buffer, strlen(buffer), 0);

        if (written_bytes < 0)
            error("ERROR writing to socket");
    }
}

void ChatClient::handleNewConnection() {
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    struct auth_packet packet;
    int newsockfd, read_bytes;
    User crt_user;


    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd == -1) { 
        error("ERROR in accept");
    } else { 
        read_bytes = recv(newsockfd, &packet, sizeof(packet), 0);
        if (read_bytes < 0)
            error("ERROR writing to socket");

        #ifdef DEBUG
            printf("\ndebug: new client connected to this client's chat server\n");
            printf("debug: info for the new client ->\n");
            printf("debug: <- name=%s address=%s port=%d\n", packet.name, inet_ntoa(cli_addr.sin_addr), packet.port);
            printf("debug> ");
            fflush(stdout);
        #endif

        crt_user = findUser(packet.name);

        #ifdef DEBUG
            if(crt_user == NULL) {
                printf("\ndebug: the client trying to connect is not authentificated with the main server\n");
                printf("debug> ");
                fflush(stdout);
            } else {
                struct user_info * crt_info = & crt_user->info;

                printf("\ndebug: the client trying to connect is authentificated with the main server\n");
                printf("debug: info for the user in the list ->\n");
                printf("debug: <- name=%s address=%s port=%d\n", crt_info->name, inet_ntoa(crt_info->address), crt_info->port);
                printf("debug> ");
                fflush(stdout);
            }
        #endif

        if(crt_user != NULL) {
            struct user_info * crt_info = & crt_user->info;

            if(crt_info->port == packet.port && crt_info->address.s_addr == cli_addr.sin_addr.s_addr) {
                #ifdef DEBUG
                    printf("\ndebug: the client has been authentificated with us\n");
                    printf("debug: the asociated socket is #%d\n", newsockfd);
                    printf("debug> ");
                    fflush(stdout);
                #endif

                crt_user->sockfd = newsockfd;

                addReadFD(newsockfd);
            }
        }
    }
}


void ChatClient::sendFile(User crt_user, char *filename) {
    int newsockfd;

    struct sockaddr_in serv_addr;
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr = crt_user->info.address;
    serv_addr.sin_port = htons(crt_user->info.file_port);

    connectToServer(newsockfd, serv_addr);

    char *file_list[1];
    file_list[0] = strdup(filename);

    kermit_send(newsockfd, newsockfd, file_list, 1);

    close(newsockfd);

    free(file_list[0]);
}


void ChatClient::receiveFile() {
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    int newsockfd;

    clilen = sizeof(cli_addr);

    newsockfd = accept(file_sockfd, (struct sockaddr *) &cli_addr, &clilen);

    printf("\n");

    char *filename = kermit_receive(newsockfd, newsockfd, down_dir);
    if(filename != NULL) {
        files.push_back(string(filename));
        free(filename);
    }


    printf("client> ");
    fflush(stdout);


    close(newsockfd);
}

void ChatClient::sendAuthPacket(int auth_sockfd) {
    struct auth_packet packet;
    int bytes_written;

    memset((char *) &packet, 0, sizeof(auth_packet));
    strcpy(packet.name, name);
    packet.port = portno;
    packet.file_port = file_portno;

    bytes_written = send(auth_sockfd, &packet, sizeof(struct auth_packet), 0);
    if (bytes_written < 0)
        error("ERROR writing to socket");
}
