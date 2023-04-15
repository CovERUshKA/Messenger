#pragma once

#include <string>
#include <vector>

struct Message {
    int message_id;
    int from;
    std::string text;
};

struct Chat {
    int chat_id;
    std::string name;
    std::vector<Message> messages;
};

class User {
public:
    std::string sLastError;

    int iUserId;
    std::string sUsername;
    std::string sSessionKey;

    Network network;

    std::vector<Chat> chats;

    User();

    bool Login(std::string username, std::string password);
    void Logout();
    bool Register(std::string username, std::string password, std::string password_confirmation);

    bool LoadChats();

    bool AddMessageToChat(int chat_id, std::string chat_name, nlohmann::json message);

    bool SendMessageToChat(int chat_id, std::string sMessage);
};