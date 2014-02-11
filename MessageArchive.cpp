#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <utility>
#include "MessageArchive.h"
#include "commons.h"

using namespace std;

bool MessageArchive::addMessage(const struct user_info &info, const char* message) {
    FILE *f;
    char buffer[256];
    char port_str[50];
    bool ret_val;

    sprintf(port_str, "%d", info.port);

    strcpy(buffer, "log_");
    strcat(buffer, info.name);
    strcat(buffer, "_");
    strcat(buffer, inet_ntoa(info.address));
    strcat(buffer, "_");
    strcat(buffer, port_str);

    f = fopen(buffer, "at");

    if(msg[info].size() == 0 || msg[info].back().back().compare(".") == 0) {
        msg[info].push_back(vector<string> ());

        fprintf(f, "<mesaj %d>\n", msg[info].size());

        #ifdef DEBUG
            printf("\ndebug (archive): a whole other message is being received\n");
            printf("debug> ");
            fflush(stdout);
        #endif
    }

    msg[info].back().push_back(string(message));

    ret_val = (strcmp(message, ".") == 0);

    #ifdef DEBUG
        printf("\ndebug (archive): new message line added to the archive\n");
        printf("debug (archive): info for the sender (from the list) ->\n");
        printf("debug (archive): <- name=%s address=%s port=%d\n", info.name, inet_ntoa(info.address), info.port);
        printf("debug (archive): the message received reads \"%s\"\n", message);
        printf("debug> ");
        fflush(stdout);
    #endif

    if(ret_val)
        printf("\nA fost primit mesaj din partea utilizatorului %s...\n", info.name);
    else
        fprintf(f, "%s\n", message);

    fclose(f);

    return ret_val;
}


void MessageArchive::resetMessagesFrom(const struct user_info &info) {
    msg.erase(info);
}


void MessageArchive::printAllAvailableMessages() {
    map<struct user_info, vector < vector < string > >, less_info>::iterator iter;

    for(iter = msg.begin(); iter != msg.end(); ++iter) {
        int no = (*iter).second.size();

        if(no == 1)
            printf("%s: %d mesaj\n", (*iter).first.name, no);
        else
            printf("%s: %d mesaje\n", (*iter).first.name, no);
    }
}


void MessageArchive::printAvailableMessagesFrom(const struct user_info &info) {
    int no = msg[info].size();

    if(no == 1)
        printf("%s: %d mesaj\n", info.name, no);
    else
        printf("%s: %d mesaje\n", info.name, no);
}


void MessageArchive::printAvailableMessagesFrom(const char* name) {
    map<struct user_info, vector < vector < string > >, less_info>::iterator iter;

    for(iter = msg.begin(); iter != msg.end(); ++iter)
        if(strcmp((*iter).first.name, name) == 0) {
            int no = (*iter).second.size();

            if(no == 1)
                printf("%s: %d mesaj\n", (*iter).first.name, no);
            else
                printf("%s: %d mesaje\n", (*iter).first.name, no);
        }
}


void MessageArchive::printMessageFromNo(const struct user_info &info, int number) {
    int size;

    vector < vector < string > > all_msgs = msg[info];
    size = all_msgs.size();

    if(number <= 0 || number > size) {
        printf("Comanda invalida. Mesajele userului %s sunt numerotate de la 1 la %d\n", info.name, size);
        return;
    }

    vector<string> text = all_msgs[number - 1];
    size = text.size();

    for(int i = 0; i < size - 1; i++)
        printf("%s\n", text[i].c_str());

    const char *str = text[size - 1].c_str();

    if(strcmp(str, ".") != 0)
        printf("%s\n", str);
}
