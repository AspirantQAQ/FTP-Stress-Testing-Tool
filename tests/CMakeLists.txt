add_executable(test_utils
    test_utils.cpp
    ../src/utils.cpp
)
target_include_directories(test_utils PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_utils PRIVATE)
if(UNIX)
    target_link_libraries(test_utils PRIVATE pthread)
endif()

add_executable(test_config
    test_config.cpp
    ../src/config.cpp
    ../src/utils.cpp
)
target_include_directories(test_config PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_config PRIVATE)
if(UNIX)
    target_link_libraries(test_config PRIVATE pthread)
endif()

add_executable(test_stats_collector
    test_stats_collector.cpp
    ../src/stats_collector.cpp
)
target_include_directories(test_stats_collector PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_stats_collector PRIVATE)
if(UNIX)
    target_link_libraries(test_stats_collector PRIVATE pthread)
endif()

add_test(NAME test_utils COMMAND test_utils)
add_test(NAME test_config COMMAND test_config)
add_test(NAME test_stats_collector COMMAND test_stats_collector)
