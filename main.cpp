#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
 
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* buffer) {
    size_t newLength = size * nmemb;
    buffer->append((char*)contents, newLength);
    return newLength;
}
 
int main() {
    std::string logID = "3732357";
    std::string playerUID = "[U:1:431667580]"; 
 
    // http://logs.tf/api/v1/log?title=X&uploader=Y&player=Z&limit=N&offset=N 
    std::string apiUrl = "https://logs.tf/api/v1/log/" + logID;

    // dewe's UID
    // [U:1:431667580]

 
    CURL* curl;
    CURLcode res;
    std::string responseString;
 
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
 
        res = curl_easy_perform(curl);
 
        if(res != CURLE_OK) {
            std::cerr << "Failed." << curl_easy_strerror(res) << std::endl;
        } else {
            auto jsonResponse = nlohmann::json::parse(responseString);
 
            float DPM = jsonResponse["players"][playerUID]["dapm"];
            std::string tempString = "The DPM you hit in this log was " + std::to_string(DPM) + ".";
 
            std::cout << tempString << std::endl;
        }
 
        curl_easy_cleanup(curl);
    }
 
    curl_global_cleanup();
    std::cin.get();
    return 0;
}