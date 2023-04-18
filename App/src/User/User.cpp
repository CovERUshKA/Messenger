#include "../Network/Network.h"
#include "User.h"

Message JsonMessageToMessage(nlohmann::json message)
{
    int message_id = message["message_id"].get<int>();
    int from = message["from"].get<int>();
    std::string text = message["text"].get<std::string>();

    return Message{message_id, from, text};
}

std::vector<User> User::GetKnownUsers()
{
    std::vector<User> known;
    for (auto& chat : chats) {
        if (chat.chat_id > 0)
        {
            known.push_back({chat.chat_id, chat.name});
        }
    }

    return known;
}

bool User::LoadChats()
{
    nlohmann::json chats_json;

    if (network.get_api_with_arguments("getChats", "", chats_json))
    {
        chats_json = chats_json["result"]["chats"];
        
        // range-based for
        for (auto& chat : chats_json) {
            if (chat.contains("chat_id")
                && chat.contains("name")
                && chat.contains("messages"))
            {
                Chat new_chat;
                new_chat.chat_id = chat["chat_id"].get<int>();
                new_chat.name = chat["name"].get<std::string>();

                if (new_chat.chat_id < 0)
                {
                    auto users_json = chat["users"];

                    for (size_t i = 0; i < users_json.size();)
                    {
                        nlohmann::json user = users_json[0];
                        users_json.erase(0);

                        int iUserId = user["user_id"].get<int>();
                        std::string sUsername = user["username"].get<std::string>();

                        new_chat.users.push_back({iUserId, sUsername});
                    }
                }

                auto messages_json = chat["messages"];

                for (size_t i = 0; i < messages_json.size();)
                {
                    nlohmann::json message = messages_json[0];
                    messages_json.erase(0);

                    int message_id = message["message_id"].get<int>();
                    int from = message["from"].get<int>();
                    std::string text = message["text"].get<std::string>();

                    new_chat.messages.push_back({message_id, from, text});
                }

                chats.push_back(new_chat);
            }
            //std::cout << element << '\n';
        }

        return true;
    }

    return false;
}

bool User::AddMessageToChat(int chat_id, std::string chat_name, nlohmann::json message)
{
    // range-based for
    for (auto& chat : chats) {
        if (chat.chat_id == chat_id)
        {
            chat.messages.push_back(JsonMessageToMessage(message));
            return true;
        }
    }

    Chat chat;

    chat.chat_id = chat_id;
    chat.name = chat_name;
    chat.messages.push_back(JsonMessageToMessage(message));

    chats.push_back(chat);

    return false;
}

void User::Logout()
{
    iUserId = -1;
    sUsername = "";
    sSessionKey = "";
    sLastError = "";

    chats.clear();
    network.clear_header();
}

bool User::SendMessageToChat(int chat_id, std::string sMessage)
{
    nlohmann::json result;
    std::string query_arguments("chat_id=");

    //MessageBoxA(0, "2", "2", 0);

    query_arguments.append(std::to_string(chat_id));

    //MessageBoxA(0, "3", "3", 0);

    query_arguments.append("&text=");

    //MessageBoxA(0, "4", "4", 0);

    query_arguments.append(network.encode_string(sMessage));

    //MessageBoxA(0, "5", "5", 0);

    network.get_api_with_arguments("sendMessage", query_arguments.c_str(), result);

    //MessageBoxA(0, "7", "7", 0);

    return true;
}

std::vector<User> User::SearchUsers(std::string input_search)
{
    std::vector<User> found_users;
    nlohmann::json result;
    std::string query_arguments("data=");

    query_arguments.append(input_search);

    if (input_search.length() != 0 && network.get_api_with_arguments("search", query_arguments.c_str(), result))
    {
        if (result["ok"] == true)
        {
            auto users_json = result["result"]["users"];

            // range-based for
            for (auto& user : users_json) {
                if (user.contains("user_id") && user.contains("username"))
                {
                    int user_id = user["user_id"].get<int>();
                    std::string username = user["username"].get<std::string>();

                    found_users.push_back({user_id, username});
                }
            }
        }
    }

    return found_users;
}

bool User::CreateGroup(std::string sGroupName, std::vector<int> iUsersId)
{
    nlohmann::json result;
    nlohmann::json body;

    body["name"] = sGroupName;
    body["users"] = iUsersId;

    // username=KEK&password=12345
    //auto success = network.get_api_with_arguments("login", login_query.c_str(), result);
    auto success = network.post_api_with_json("createGroup", body.dump().c_str(), result);

    //MessageBoxA(0, result.dump().c_str(), 0, 0);

    if (success)
    {
        if (result["ok"] == true)
        {
            auto data = result["result"];

            Chat chat;

            chat.chat_id = data["chat_id"].get<int>();
            chat.name = data["name"].get<std::string>();
            
            auto users_json = data["users"];

            for (size_t i = 0; i < users_json.size();)
            {
                nlohmann::json user = users_json[0];
                users_json.erase(0);

                int iUserId = user["user_id"].get<int>();
                std::string sUsername = user["username"].get<std::string>();

                chat.users.push_back({iUserId, sUsername});
            }

            chats.push_back(chat);

            return true;
        }
        else if (result["ok"] == false)
        {
            sLastError = result["description"];
        }
    }
    else
    {
        sLastError = result["description"];
    }

    return false;
}

bool User::Register(std::string username, std::string password, std::string password_confirmation)
{
    nlohmann::json response;
    nlohmann::json register_body;

    if (password.compare(password_confirmation) == 0)
    {
        register_body["username"] = username;
        register_body["password"] = password;

        //MessageBoxA(0, register_body.dump().c_str(), 0, 0);

        // username=KEK&password=12345
        if (network.post_api_with_json("createAccount", register_body.dump().c_str(), response))
        {
            if(response["ok"] == true)
            {
                auto user_data = response["result"];
                //MessageBoxA(0, user_data.dump().c_str(), 0, 0);
                
                iUserId = user_data["user_id"].get<int>();
                sUsername = user_data["username"].get<std::string>();
                sSessionKey = user_data["session_key"].get<std::string>();

                network.add_header(std::string("Authorization: Bearer ") + sSessionKey);

                return true;
            }
            else
            {
                sLastError = response["description"];
            }
        }
        else
        {
            sLastError = response["description"];
        }
    }
    else
    {
        sLastError = "Passwords doesn't equal";
    }

    return false;
}

bool User::Login(std::string username, std::string password)
{
    nlohmann::json result;
    nlohmann::json login_body;

    login_body["username"] = username;
    login_body["password"] = password;

    //MessageBoxA(0, login_body.dump().c_str(), 0, 0);

    // username=KEK&password=12345
    //auto success = network.get_api_with_arguments("login", login_query.c_str(), result);
    auto success = network.post_api_with_json("login", login_body.dump().c_str(), result);

    //MessageBoxA(0, result.dump().c_str(), 0, 0);

    if (success)
    {
        if (result["ok"] == true)
        {
            auto user_data = result["result"];

            iUserId = user_data["user_id"].get<int>();
            sUsername = user_data["username"].get<std::string>();
            sSessionKey = user_data["session_key"].get<std::string>();

            network.add_header(std::string("Authorization: Bearer ") + sSessionKey);

            return true;
        }
        else if (result["ok"] == false)
        {
            sLastError = result["description"];
        }
    }
    else
    {
        sLastError = result["description"];
    }

    return false;
}

User::User(int iUserId, std::string sUsername)
{
    this->iUserId = iUserId;
    this->sUsername = sUsername;
}

User::User() : iUserId(-1)
{
    network.Initialize();
}

std::string Chat::GetName(int iUserId)
{
    for (auto& user : users)
        if (user.iUserId == iUserId)
            return user.sUsername;

    return "Unknown";
}