#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <chrono>
#include <thread>

#include <algorithm>

#include <curl/curl.h>
#include <nlohmann/json.hpp>
 
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* buffer) {
    size_t newLength = size * nmemb;
    buffer->append((char*)contents, newLength);
    return newLength;
}

std::vector<std::string> getTotalLogs() {
    std::vector<std::string> logIDs; 
    const std::string playerID = "76561198108292940"; 
    const std::string apiUrl = "https://logs.tf/api/v1/log?player=" + playerID;

    CURL* curl = curl_easy_init();
    std::string responseString;
 
    curl_global_init(CURL_GLOBAL_DEFAULT);

    int totalLogs = 0;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
 
        CURLcode res = curl_easy_perform(curl);
 
        if(res != CURLE_OK) {
            std::cerr << "Failed." << curl_easy_strerror(res) << std::endl;
        } else {

            try {
                auto jsonResponse = nlohmann::json::parse(responseString);
                totalLogs = jsonResponse["total"]; 

                for(int i = 0; i < totalLogs - 1; i++) {
                    const auto& log = jsonResponse["logs"][i];
                    if (log["players"] == 12 && log["id"] > 2672567) {
                        logIDs.push_back(std::to_string(log["id"].get<int>()));
                    }
                }
            } catch(const nlohmann::json::exception& e) {
                std::cerr << "JSON Error: " << e.what() << std::endl; 
            }
         }
 
        curl_easy_cleanup(curl);
    }
 
    curl_global_cleanup();
    return logIDs; 
}

std::vector<std::pair<std::string, float>> getTopFiveWorstLogs(std::vector<std::pair<std::string, float>> ratioVectorDPM) {
    std::sort(ratioVectorDPM.begin(), ratioVectorDPM.end(), [](const std::pair<std::string, float>& a, const std::pair<std::string, float>& b) {
            return a.second < b.second; 
    });

    if (ratioVectorDPM.size() > 5) {
        ratioVectorDPM.resize(5);
    }

    return ratioVectorDPM; 
}

std::vector<std::pair<std::string, float>> getDPM() {
    const std::string playerUID = "[U:1:148027212]"; 
    const std::vector<std::string> logIDs = getTotalLogs();
    const size_t totalLogs = logIDs.size(); 
    const size_t batchSize = 8; 

    std::vector<std::pair<std::string, float>> ratioVectorDPM; 

    CURLM* multi_handle;
    multi_handle = curl_multi_init();

    for (size_t start = 0; start < totalLogs; start += batchSize) {
        size_t curBatchSize = 0;
        if (batchSize < (logIDs.size() - start)) {
            curBatchSize = batchSize; 
        } else {
            curBatchSize = logIDs.size() - start; 
        }

        std::vector<CURL*> curls(curBatchSize);
        std::vector<std::string> responseStrings(curBatchSize);  

        for (int i = 0; i < curBatchSize; i++) {
            std::string apiUrl = "https://logs.tf/api/v1/log/" + logIDs[start + i];;

            curls[i] = curl_easy_init();
            curl_easy_setopt(curls[i], CURLOPT_URL, apiUrl.c_str());
            curl_easy_setopt(curls[i], CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curls[i], CURLOPT_WRITEDATA, &responseStrings[i]);
            curl_multi_add_handle(multi_handle, curls[i]);
        }

        int still_running = 0;
        curl_multi_perform(multi_handle, &still_running);

        while (still_running) {
            curl_multi_wait(multi_handle, nullptr, 0, 1000, nullptr);
            curl_multi_perform(multi_handle, &still_running);
        }

        for(int i = 0; i < curBatchSize; i++) {        
            curl_multi_remove_handle(multi_handle, curls[i]);
            curl_easy_cleanup(curls[i]);

            try {
                auto jsonResponse = nlohmann::json::parse(responseStrings[i]);

                auto allPlayers = jsonResponse["players"];
                int userDPM = jsonResponse["players"][playerUID]["dapm"];
    
                std::vector<int> allPlayerDPM; 
                for (nlohmann::json::iterator it = allPlayers.begin(); it != allPlayers.end(); ++it) {
                    auto player = it.value();
                    if(player["class_stats"][0]["type"] != "medic" && player["class_stats"][0]["total_time"] > 1200) {
                        allPlayerDPM.push_back(player["dapm"]); 
                    }
                }

                if (!allPlayerDPM.empty() && jsonResponse["players"][playerUID]["class_stats"][0]["type"] != "medic") { 
                    float avgDPM = std::accumulate(allPlayerDPM.begin(), allPlayerDPM.end(), 0.0f) / allPlayerDPM.size();
                    float ratioDPM = userDPM / avgDPM; 
                    // std::cout << "Log ID: " << logIDs[start + i] << ", DPM: " <<  ratioDPM << std::endl;
                    ratioVectorDPM.push_back(std::make_pair(logIDs[start + i], ratioDPM));  
                }
            } catch(const nlohmann::json::exception& e) {
                std::cerr << "JSON Error: " << e.what() << std::endl; 
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1)); 
    }

    curl_multi_cleanup(multi_handle); 

    return getTopFiveWorstLogs(ratioVectorDPM); 
}

int main() {
    std::vector<std::pair<std::string, float>> ratioVectorDPM = getDPM(); 

    for(const std::pair<std::string, float>& ratioDPM : ratioVectorDPM) {
        std::cout << "https://logs.tf/" << ratioDPM.first << ", DPM: " <<  ratioDPM.second << std::endl; 
    }
    std::cout << "fin" << std::endl; 
}