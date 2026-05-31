cmake_minimum_required(VERSION 3.20)

project(PearToPearRelay LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(PEAR_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    PearToPear
    GIT_REPOSITORY https://github.com/p2pSquad/PearToPear.git
    GIT_TAG feature/transport-abstraction
)

FetchContent_MakeAvailable(PearToPear)

add_library(pear_relay
    src/relay_transport.cpp
)

target_include_directories(pear_relay PUBLIC
    include
)

target_link_libraries(pear_relay PUBLIC
    pear_net
)

add_executable(pear_relay_server
    apps/relay_server/main.cpp
)

target_link_libraries(pear_relay_server PRIVATE
    pear_relay
)

add_executable(pear_relay_demo
    apps/relay_demo/main.cpp
)

target_link_libraries(pear_relay_demo PRIVATE
    pear_relay
)
