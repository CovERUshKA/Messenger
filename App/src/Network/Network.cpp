#include "../config.h"
#include "Network.h"

// функция, вызываемая cURL для записи полученых данных
size_t curl_cb(char *data, size_t size, size_t nmemb, std::string *buffer)  
{  
    size_t result = 0;  

    if (buffer != NULL)  
    {  
            buffer->append(data, size * nmemb);  
            result = size * nmemb;  
    }
    
    return result;  
}

char* Network::encode_string(std::string str)
{
    return curl_easy_escape(curl, str.c_str(), str.length());
}

bool Network::get_api_with_arguments(std::string method, std::string arguments, nlohmann::json& json_result)
{
    std::string result;

    //MessageBoxA(0, "333", "333", 0);

    if (!get_api_with_arguments(method, arguments, result))
    {
        json_result["ok"] = false;
        json_result["error_code"] = 0;
        json_result["description"] = result;
        return false;
    }

    //MessageBoxA(0, "444", "444", 0);

    //MessageBoxA(0, result.c_str(), 0, 0);

    try
    {
        json_result = nlohmann::json::parse(result.c_str());
    }
    catch(const nlohmann::json::exception& e)
    {
        json_result["ok"] = false;
        json_result["error_code"] = 0;
        json_result["description"] = e.what();
        return false;
    }
    
    return true;
}

bool Network::get_api_with_arguments(std::string method, std::string arguments, nlohmann::ordered_json& json_result)
{
    std::string result;

    if (!get_api_with_arguments(method, arguments, result))
    {
        json_result["ok"] = false;
        json_result["error_code"] = 0;
        json_result["description"] = result;
        return false;
    }

    try
    {
        json_result = nlohmann::ordered_json::parse(result.c_str());
    }
    catch(const nlohmann::json::exception& e)
    {
        json_result["ok"] = false;
        json_result["error_code"] = 0;
        json_result["description"] = e.what();
        return false;
    }

    return true;
}

bool Network::post_api_with_json(std::string method, std::string json_data, nlohmann::json& json_result)
{
    std::string result;

    if (!post_api_with_json(method, json_data, result))
    {
        json_result["ok"] = false;
        json_result["error_code"] = 0;
        json_result["description"] = result;
        return false;
    }

    try
    {
        json_result = nlohmann::ordered_json::parse(result.c_str());
    }
    catch(const nlohmann::json::exception& e)
    {
        json_result["ok"] = false;
        json_result["error_code"] = 0;
        json_result["description"] = e.what();
        return false;
    }

    return true;
}

bool Network::post_api_with_json(std::string method, std::string json_data, std::string& str_result)
{
    CURLcode curlResult;

    std::string url(apiurl);

    url += method;

    if( !curl ) {
        str_result = "curl is empty";
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST" );
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.length());
    
    auto curl_headers = LoadHeaders();

    // the actual code has the actual token in place of <my_token>
    curl_headers = curl_slist_append(curl_headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);

    //MessageBoxA(0, url.c_str(), "Url", 0);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str_result);
    //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

    // This sometimes cause curl_easy_perform to fail, idk why this been set here
    //curl_easy_setopt(BinanceAPI::curl, CURLOPT_ENCODING, "gzip");

    curlResult = curl_easy_perform(curl);

    curl_slist_free_all(curl_headers);

    //MessageBoxA(0, str_result.c_str(), "Unsuccess curl", 0);

    /* Check for errors */ 
    if ( curlResult != CURLE_OK ) {
        str_result = curl_easy_strerror(curlResult);
        //MessageBoxA(0, curl_easy_strerror(curlResult), "Unsuccess curl", 0);
        return false;
        //BinanceAPI_logger::write_log( "<BinanceAPI::curl_api_with_header> curl_easy_perform() failed: %s" , curl_easy_strerror(curlResult) );
    }

    //BinanceAPI_logger::write_log( "<BinanceAPI::curl_api_with_header> Done" );

    return true;
}

//--------------------
// Do the curl
bool
Network::get_api_with_arguments(std::string method, std::string arguments, std::string& str_result)
{
    CURLcode curlResult;

    std::string url(apiurl);
    //MessageBoxA(0, "01", "02", 0);
    url += method;
    url += "?";
    url += arguments;
    //MessageBoxA(0, "00", "00", 0);
    if( !curl ) {
        str_result = "curl is empty";
        return false;
    }

    //MessageBoxA(0, "111", "111", 0);

    //MessageBoxA(0, url.c_str(), "Url", 0);
    curlResult = curl_easy_setopt(curl, CURLOPT_POST, 0);
    curlResult = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    //MessageBoxA(0, "22", "22", 0);
    //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_cb);
    curlResult = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str_result);
    //MessageBoxA(0, "33", "33", 0);
    //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    // Используем метод GET в api
    curlResult = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET" );
    curlResult = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, 0);
    curlResult = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);
    //MessageBoxA(0, "44", "44", 0);

    auto curl_headers = LoadHeaders();
    curlResult = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);

    //MessageBoxA(0, curl_easy_strerror(curlResult), "Unsuccess curl", 0);

    // This sometimes cause curl_easy_perform to fail, idk why this been set here
    //curl_easy_setopt(BinanceAPI::curl, CURLOPT_ENCODING, "gzip");

    curlResult = curl_easy_perform(curl);

    curl_slist_free_all(curl_headers);

    //MessageBoxA(0, "55", "55", 0);

    /* Check for errors */ 
    if ( curlResult != CURLE_OK ) {
        str_result = curl_easy_strerror(curlResult);
        //MessageBoxA(0, curl_easy_strerror(curlResult), "Unsuccess curl", 0);
        return false;
        //BinanceAPI_logger::write_log( "<BinanceAPI::curl_api_with_header> curl_easy_perform() failed: %s" , curl_easy_strerror(curlResult) );
    }

    //BinanceAPI_logger::write_log( "<BinanceAPI::curl_api_with_header> Done" );

    return true;
}

bool Network::Initialize()
{
    try
    {
        curl = curl_easy_init();
        if (!curl) {
            throw std::exception("Unable to initialize curl");
        }

        // Set buffer for errors
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlErrorBuffer);
        
        // переходить по "Location:" указаному в HTTP заголовке  
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        // не проверять сертификат удаленного сервера
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        // не проверять сертификат удаленного сервера
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        // использовать метод POST для отправки данных
        //curl_easy_setopt(curl, CURLOPT_POST, 1);
        // параметры POST
        //curl_easy_setopt(curl, CURLOPT_POSTFIELDS, urlPOST);
        // функция, вызываемая cURL для записи полученых данных
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_cb);

        this->initialized = true;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return this->initialized;
}

curl_slist* Network::LoadHeaders()
{
    curl_slist* curl_headers = nullptr;
    for (auto& header : headers)
    {
        curl_headers = curl_slist_append(curl_headers, header.c_str());
    }

    return curl_headers;
}

bool Network::clear_header()
{
    CURLcode curlResult;

    headers.clear();

    curlResult = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, 0);

    /* Check for errors */ 
    if ( curlResult != CURLE_OK ) {
        //str_result = curl_easy_strerror(curlResult);
        MessageBoxA(0, curl_easy_strerror(curlResult), "Unsuccess curl headers clearing", 0);
        return false;
        //BinanceAPI_logger::write_log( "<BinanceAPI::curl_api_with_header> curl_easy_perform() failed: %s" , curl_easy_strerror(curlResult) );
    }

    return true;
}

bool Network::add_header(std::string data)
{
    headers.push_back(data);
    // the actual code has the actual token in place of <my_token>
    // headers = curl_slist_append(headers, data.c_str());

    // curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    return true;
}

bool Network::IsInitialized()
{
    return this->initialized;
}

Network::~Network()
{
    // завершение сеанса
    curl_easy_cleanup(curl);
}
