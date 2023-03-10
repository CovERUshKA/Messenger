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

    if (!curl_api_with_arguments(method, arguments, result))
    {
        json_result["ok"] = false;
        json_result["error_code"] = 0;
        json_result["description"] = result;
        return false;
    }

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

    if (!curl_api_with_arguments(method, arguments, result))
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

//--------------------
// Do the curl
bool
Network::curl_api_with_arguments(std::string method, std::string arguments, std::string& str_result)
{
    CURLcode curlResult;

    std::string url(apiurl);

    url += method;
    url += "?";
    url += arguments;

    if( curl ) {
        //MessageBoxA(0, url.c_str(), "Url", 0);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str() );
        //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str_result );
        //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

        // This sometimes cause curl_easy_perform to fail, idk why this been set here
        //curl_easy_setopt(BinanceAPI::curl, CURLOPT_ENCODING, "gzip");

        curlResult = curl_easy_perform(curl);

        /* Check for errors */ 
        if ( curlResult != CURLE_OK ) {
            str_result = curl_easy_strerror(curlResult);
            //MessageBoxA(0, curl_easy_strerror(curlResult), "Unsuccess curl", 0);
            return false;
            //BinanceAPI_logger::write_log( "<BinanceAPI::curl_api_with_header> curl_easy_perform() failed: %s" , curl_easy_strerror(curlResult) );
        } 	

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
        // Используем метод GET в api
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET" );
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

bool Network::IsInitialized()
{
    return this->initialized;
}

Network::~Network()
{
    // завершение сеанса
    curl_easy_cleanup(curl);
}