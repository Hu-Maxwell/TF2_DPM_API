cmake_minimum_required(VERSION 3.10)
project(WeatherFetch)
 
# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
 
# Find libcurl
find_package(CURL REQUIRED)
 
# Include nlohmann JSON (header-only, so no linking needed)
find_path(JSON_INCLUDE_DIR "nlohmann/json.hpp")
 
# Add the executable
add_executable(WeatherFetch main.cpp)
 
# Include directories for nlohmann JSON
target_include_directories(WeatherFetch PRIVATE ${JSON_INCLUDE_DIR})
 
# Link libcurl
target_link_libraries(WeatherFetch PRIVATE CURL::libcurl)