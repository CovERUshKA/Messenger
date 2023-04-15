#include "../Network/Network.h"
#include "User.h"

Message JsonMessageToMessage(nlohmann::json message)
{
    int message_id = message["message_id"].get<int>();
    int from = message["from"].get<int>();
    std::string text = message["text"].get<std::string>();

    return Message{message_id, from, text};
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
                int chat_id = chat["chat_id"].get<int>();
                std::string chat_name = chat["name"].get<std::string>();

                auto messages_json = chat["messages"];

                std::vector<Message> messages;

                for (size_t i = 0; i < messages_json.size();)
                {
                    nlohmann::json message = messages_json[0];
                    messages_json.erase(0);

                    int message_id = message["message_id"].get<int>();
                    int from = message["from"].get<int>();
                    std::string text = message["text"].get<std::string>();

                    messages.push_back({message_id, from, text});
                }

                chats.push_back({chat_id, chat_name, messages});
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

    chats.clear();
}

bool User::SendMessageToChat(int chat_id, std::string sMessage)
{
    nlohmann::json result;
    std::string query_arguments("chat_id=");

    query_arguments.append(std::to_string(chat_id));

    query_arguments.append("&text=");

    query_arguments.append(network.encode_string(sMessage));

    network.get_api_with_arguments("sendMessage", query_arguments.c_str(), result);

    return true;
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
                MessageBoxA(0, user_data.dump().c_str(), 0, 0);
                
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

User::User() : iUserId(-1)
{
    network.Initialize();
}