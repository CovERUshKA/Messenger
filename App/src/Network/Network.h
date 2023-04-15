#pragma once

#include <iostream>
#include <WinSock2.h>
#include <curl/curl.h>

#include "single_include/nlohmann/json.hpp"

class Network
{
public:
    Network() : initialized(false), curl(nullptr) {}
    ~Network();

    bool Initialize();
    bool IsInitialized();

    bool add_header(std::string data);

    bool get_api_with_arguments(std::string method, std::string arguments, std::string& str_result);
    bool get_api_with_arguments(std::string method, std::string arguments, nlohmann::json& str_result);
    bool get_api_with_arguments(std::string method, std::string arguments, nlohmann::ordered_json& str_result);

    bool post_api_with_json(std::string method, std::string json_data, std::string& str_result);
    bool post_api_with_json(std::string method, std::string json_data, nlohmann::json& json_result);

    char* encode_string(std::string str);

private:
    bool initialized;

    CURL *curl;

    // буфер для сохранения текстовых ошибок
    char curlErrorBuffer[CURL_ERROR_SIZE];
};
