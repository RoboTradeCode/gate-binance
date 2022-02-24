cmake_minimum_required(VERSION 3.12)

project(BinanceConnector)

set(CMAKE_CXX_STANDARD 20)

set(SRC_DIR src)

# библиотека BinaCPP
set(BINACPP_SRC libs/binacpp/src)
set(BINACPP_LIB_DIR libs/binacpp/lib/libbinacpp/lib)
set(WEBSOCKETS_DIR libs/binacpp/lib/libwebsockets-2.4.0)
set(JSONCPP_SRC libs/binacpp/lib/jsoncpp-1.8.3/include)

# библиотека aeron
set(AERON_DIR ../aeron)

set(AERON_CPP_DIR libs/aeron_cpp/src)

include_directories(${AERON_DIR}/aeron-client/src/main/cpp)

find_library(AERON_CLIENT_LIB aeron_client ${AERON_DIR}/cppbuild/Release/lib/)

find_library(WEBSOCKETS_LIB websockets ${WEBSOCKETS_DIR}/lib)

find_package(jsoncpp REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(
        ${PROJECT_NAME}
        #        main file, entry point of gateway
        main.cpp
        #        toml++ library for config
        libs/tomlplusplus/include/toml++/toml.h

        ${WEBSOCKETS_DIR}/include
        #        own library for working with aeron
        ${AERON_CPP_DIR}/Publisher.cpp
        ${AERON_CPP_DIR}/Publisher.h
        ${AERON_CPP_DIR}/Subscriber.cpp
        ${AERON_CPP_DIR}/Subscriber.h
        #        sources
        ${SRC_DIR}/ws_handlers.cpp
        ${SRC_DIR}/ws_handlers.h
        ${SRC_DIR}/utils.cpp
        ${SRC_DIR}/utils.h
        ${SRC_DIR}/aeron_connectors.cpp
        ${SRC_DIR}/aeron_connectors.h
        ${SRC_DIR}/order_handlers.h
        ${SRC_DIR}/order_handlers.cpp
        ${SRC_DIR}/gate_config.h
        ${SRC_DIR}/aeron_channel.h
        #        BinaCPP library
        ${BINACPP_SRC}/binacpp.cpp
        ${BINACPP_SRC}/binacpp_logger.cpp
        ${BINACPP_SRC}/binacpp_utils.cpp
        ${BINACPP_SRC}/binacpp_websocket.cpp
        src/global_config.h)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${SRC_DIR}
        #
        #        websockets library, for BinaCPP lib
        ${WEBSOCKETS_DIR}/include
        #        JsonCpp library, for BinaCPP lib
        ${JSONCPP_SRC}
        ${AERON_UTILS_DIR}
        ${BINACPP_SRC}
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        jsoncpp
        ${WEBSOCKETS_LIB}
        pthread
        curl
        OpenSSL::SSL
        ${AERON_CLIENT_LIB}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.toml ${CMAKE_CURRENT_BINARY_DIR}/config.toml COPYONLY)
