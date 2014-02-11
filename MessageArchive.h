#ifndef MESSAGE_ARCHIVE_H_
#define MESSAGE_ARCHIVE_H_

#include <netinet/in.h>
#include <cstring>
#include <vector>
#include <string>
#include <map>
#include "commons.h"

class MessageArchive {
    private:
        std::map<struct user_info, std::vector < std::vector < std::string > >, less_info> msg;

    public:
        MessageArchive() {}
        ~MessageArchive() {}

        bool addMessage(const struct user_info &info, const char* message);
        void resetMessagesFrom(const struct user_info &info);
        void printAllAvailableMessages();
        void printAvailableMessagesFrom(const struct user_info &info);
        void printAvailableMessagesFrom(const char* name);
        void printMessageFromNo(const struct user_info &info, int number);
};

#endif /*MESSAGE_ARCHIVE_H_*/

