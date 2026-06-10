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
    GIT_TAG main
)

FetchContent_MakeAvailable(PearToPear)

find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets)

pkg_check_modules(GRPC REQUIRED IMPORTED_TARGET grpc++)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

set(RELAY_PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/proto/relay.proto)
set(RELAY_PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${RELAY_PROTO_BINARY_DIR})

set(RELAY_PROTO_SRCS ${RELAY_PROTO_BINARY_DIR}/relay.pb.cc)
set(RELAY_PROTO_HDRS ${RELAY_PROTO_BINARY_DIR}/relay.pb.h)
set(RELAY_GRPC_SRCS ${RELAY_PROTO_BINARY_DIR}/relay.grpc.pb.cc)
set(RELAY_GRPC_HDRS ${RELAY_PROTO_BINARY_DIR}/relay.grpc.pb.h)

add_custom_command(
    OUTPUT
        ${RELAY_PROTO_SRCS}
        ${RELAY_PROTO_HDRS}
        ${RELAY_GRPC_SRCS}
        ${RELAY_GRPC_HDRS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
        --cpp_out=${RELAY_PROTO_BINARY_DIR}
        --grpc_out=${RELAY_PROTO_BINARY_DIR}
        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
        ${RELAY_PROTO_FILE}
    DEPENDS
        ${RELAY_PROTO_FILE}
    VERBATIM
)

add_library(pear_relay_proto
    ${RELAY_PROTO_SRCS}
    ${RELAY_GRPC_SRCS}
)

target_include_directories(pear_relay_proto PUBLIC
    ${RELAY_PROTO_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS}
)

target_link_libraries(pear_relay_proto PUBLIC
    ${Protobuf_LIBRARIES}
    PkgConfig::GRPC
)

add_library(pear_relay
    src/relay_client.cpp
    src/relay_service.cpp
    src/relay_transport.cpp
)

target_include_directories(pear_relay PUBLIC
    include
)

target_link_libraries(pear_relay PUBLIC
    pear_net
    pear_relay_proto
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


set(PEAR_CLI_SOURCES
    ${peartopear_SOURCE_DIR}/src/pear/cli/commands.cpp
    ${peartopear_SOURCE_DIR}/src/pear/cli/command_utils.cpp
    ${peartopear_SOURCE_DIR}/src/pear/cli/sync.cpp
    ${peartopear_SOURCE_DIR}/src/pear/cli/object_download.cpp
    ${peartopear_SOURCE_DIR}/src/pear/cli/output.cpp
    ${peartopear_SOURCE_DIR}/src/pear/cli/json_output.cpp
    ${peartopear_SOURCE_DIR}/src/pear/cli/status.cpp
    ${peartopear_SOURCE_DIR}/src/pear/cli/logger.cpp
)

add_executable(pear_photo_gallery
    apps/photo_gallery/main.cpp
    apps/photo_gallery/photo_controller.cpp
    apps/photo_gallery/login_window.cpp
    apps/photo_gallery/gallery_window.cpp
    ${PEAR_CLI_SOURCES}
)

target_include_directories(pear_photo_gallery PRIVATE
    ${peartopear_SOURCE_DIR}/src/pear/cli
    ${peartopear_SOURCE_DIR}/include
)

target_compile_definitions(pear_photo_gallery PRIVATE PEAR_RELAY_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")

target_link_libraries(pear_photo_gallery PRIVATE
    pear_relay
    pear_db
    pear_fs
    pear_net
    pear_demon
    Qt5::Widgets
    nlohmann_json::nlohmann_json
)
