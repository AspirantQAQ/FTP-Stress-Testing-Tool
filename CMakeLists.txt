cmake_minimum_required(VERSION 3.14)
project(ftp_stress_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

add_executable(ftp_stress_test
    src/main.cpp
    src/config.cpp
    src/worker.cpp
    src/stats_collector.cpp
    src/reporter.cpp
    src/utils.cpp
)

target_include_directories(ftp_stress_test PRIVATE src ${CURL_INCLUDE_DIRS})
target_link_libraries(ftp_stress_test PRIVATE ${CURL_LIBRARIES})

if(UNIX)
    target_link_libraries(ftp_stress_test PRIVATE pthread)
endif()

# Testing
enable_testing()
add_subdirectory(tests)
