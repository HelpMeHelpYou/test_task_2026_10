cmake_minimum_required(VERSION 3.15)
project(udp_counter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    set(PLATFORM_LIBS ws2_32)
else()
    set(PLATFORM_LIBS "")
endif()

add_executable(udp_client client/main.cpp)
target_link_libraries(udp_client PRIVATE ${PLATFORM_LIBS})

add_executable(udp_server server/main.cpp)
target_link_libraries(udp_server PRIVATE ${PLATFORM_LIBS})

if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(udp_client PRIVATE Threads::Threads)
    target_link_libraries(udp_server PRIVATE Threads::Threads)
endif()