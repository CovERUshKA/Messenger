#pragma once

#include <string>
#include <vector>

class User;

struct Message {
    int message_id;
    int from;
    std::string text;
};

struct Chat {
    int chat_id;
    std::string name;
    std::vector<Message> messages;
    std::vector<User> users;

    std::string GetName(int iUserId);
};

class User {
public:
    std::string sLastError;

    int iUserId;
    std::string sUsername;
    std::string sSessionKey;

    Network network;

    std::vector<User> searched_users;

    std::vector<Chat> chats;

    User();

    User(int iUserId, std::string sUsername);

    bool Login(std::string username, std::string password);
    void Logout();
    bool Register(std::string username, std::string password, std::string password_confirmation);

    bool LoadChats();
    std::vector<User> GetKnownUsers();

    std::vector<User> SearchUsers(std::string text);

    bool CreateGroup(std::string sGroupName, std::vector<int> iUsersId);

    bool AddMessageToChat(int chat_id, std::string chat_name, nlohmann::json message);

    bool SendMessageToChat(int chat_id, std::string sMessage);

    inline friend bool operator==(const User& lhs, const User& rhs) { return lhs.iUserId == rhs.iUserId; }
};