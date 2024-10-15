#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <chrono>
#include <thread>

#include <curl/curl.h>
#include <nlohmann/json.hpp>
 
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* buffer) {
    size_t newLength = size * nmemb;
    buffer->append((char*)contents, newLength);
    return newLength;
}

// todo: change it so that it returns an array of all the log ids 
std::vector<int> getTotalLogs() {
    // playerID is NOT playerUID
    std::vector<int> logIDs; 
    std::string playerID = "76561198391933308"; 

    // http://logs.tf/api/v1/log?title=X&uploader=Y&player=Z&limit=N&offset=N 
    // should be limited if not using all of it! else it will be slow.  
    std::string apiUrl = "https://logs.tf/api/v1/log?player=" + playerID;

    CURL* curl;
    CURLcode res;
    std::string responseString;
 
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    int totalLogs = 0;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
 
        res = curl_easy_perform(curl);
 
        if(res != CURLE_OK) {
            std::cerr << "Failed." << curl_easy_strerror(res) << std::endl;
        } else {
            auto jsonResponse = nlohmann::json::parse(responseString);
            totalLogs = jsonResponse["total"]; 

            for(int i = 0; i < totalLogs - 1; i++) {
                logIDs.push_back(jsonResponse["logs"][i]["id"]);
            }
         }
 
        curl_easy_cleanup(curl);
    }
 
    curl_global_cleanup();
    return logIDs; 
}

std::string getDPM() {
    std::string playerUID = "[U:1:431667580]"; 
    std::vector<int> logIDs = getTotalLogs();

    // std::string DPMresults = ""; 

    const int totalLogs = logIDs.size(); // ends 1 log before first log
    const int batchSize = 8; 

    std::vector<int> DPMvector; 

    CURLM* multi_handle;
    multi_handle = curl_multi_init();

    for (int start = 0; start < totalLogs; start += batchSize) {
        int curBatchSize = 0;
        if (batchSize < (logIDs.size() - start)) {
            curBatchSize = batchSize; 
        } else {
            curBatchSize = logIDs.size() - start; 
        }

        std::vector<CURL*> curls(curBatchSize);
        std::vector<std::string> responseStrings(curBatchSize);  

        for (int i = 0; i < curBatchSize; i++) {
            int logID = logIDs[start + i];
            std::string apiUrl = "https://logs.tf/api/v1/log/" + std::to_string(logID);

            curls[i] = curl_easy_init();
            curl_easy_setopt(curls[i], CURLOPT_URL, apiUrl.c_str());
            curl_easy_setopt(curls[i], CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curls[i], CURLOPT_WRITEDATA, &responseStrings[i]);

            curl_multi_add_handle(multi_handle, curls[i]);
        }

        int still_running = 0;
        do {
            curl_multi_perform(multi_handle, &still_running);
        } while (still_running);

        std::string DPMresults = ""; 
        for(int i = 0; i < curBatchSize; i++) {        
            curl_multi_remove_handle(multi_handle, curls[i]);
            curl_easy_cleanup(curls[i]);

            auto jsonResponse = nlohmann::json::parse(responseStrings[i]);
            auto player = jsonResponse["players"][playerUID]; 
            int DPM = player["dapm"];
            DPMresults = DPMresults + "Log ID " + std::to_string(logIDs[start + i]) + ": DPM is " + std::to_string(DPM) + "\n";


            // if time is > 1200 secs, and not on medic, then add 
            if(player["class_stats"][0]["type"] != "medic" && player["class_stats"][0]["total_time"] > 1200) {
                DPMvector.push_back(DPM);
            }
        }
        std::cout << DPMresults << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1)); 
    }

    curl_multi_cleanup(multi_handle); 
    return "done"; 
}

// std::vector<int> sortDPM() {

// }

int main() {
    std::string biag = getDPM(); 
    std::cout << "fin" << std::endl; 
}