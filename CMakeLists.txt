cmake_minimum_required (VERSION 3.1)
project (snitch)

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(vendor/libtwittercpp)
add_subdirectory(vendor/yaml-cpp EXCLUDE_FROM_ALL)

include_directories(vendor/libtwittercpp/src vendor/libtwittercpp/vendor/json/src vendor/libtwittercpp/vendor/curlcpp/include vendor/yaml-cpp/include)
add_executable(snitch snitch.cpp)
set_property(TARGET snitch PROPERTY CXX_STANDARD 11)
set_property(TARGET snitch PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(snitch yaml-cpp twitter++ curlcpp curl)
