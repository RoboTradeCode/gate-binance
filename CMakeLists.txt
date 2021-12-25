cmake_minimum_required(VERSION 3.12)

project(BinanceConnector)

set(CMAKE_CXX_STANDARD 14)

set(SRC_DIR src)

# библиотека BinanceCPP
set(BINACPP_SRC lib/libbinacpp/src)
set(BINACPP_LIB_DIR lib/libbinacpp/lib/libbinacpp/lib)
set(WEBSOCKETS_DIR lib/libbinacpp/lib/libwebsockets-2.4.0)
set(JSONCPP_SRC lib/libbinacpp/lib/jsoncpp-1.8.3/include)

# библиотека aeron
set(AERON_DIR lib/aeron)
set(AERON_UTILS_DIR lib/aeron_utils)

include_directories(${AERON_DIR}/aeron-client/src/main/cpp)

find_library(AERON_CLIENT_LIB aeron_client ${AERON_DIR}/cppbuild/Release/lib/)


find_library(WEBSOCKETS_LIB websockets ${WEBSOCKETS_DIR}/lib)


find_package(jsoncpp REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(
        BinaCpp
        ${BINACPP_SRC}/binacpp.cpp
        ${BINACPP_SRC}/binacpp_logger.cpp
        ${BINACPP_SRC}/binacpp_utils.cpp
        ${BINACPP_SRC}/binacpp_websocket.cpp
        )

add_library(
        Aeron
        ${AERON_UTILS_DIR}/Publisher.cpp
        ${AERON_UTILS_DIR}/Subscriber.cpp
)
add_library(
        JsonCPP
        INTERFACE
)
add_library(
        GateLogic
        ${SRC_DIR}
        ${SRC_DIR}/aeron_connectors.cpp
        ${SRC_DIR}/order_handlers.cpp
        ${SRC_DIR}/utils.cpp
        ${SRC_DIR}/ws_handlers.cpp
)
add_library(
        WebsocketsInterface
        ${WEBSOCKETS_DIR}/include
)
add_executable(
        ${PROJECT_NAME}
        main.cpp
)

target_include_directories(
        BinaCpp
        PUBLIC
        ${BINACPP_SRC}
)
target_include_directories(
        Aeron
        PUBLIC
        ${AERON_UTILS_DIR}
)
target_include_directories(
        JsonCPP
        INTERFACE
        ${JSONCPP_SRC}
)
target_include_directories(
        GateLogic
        PUBLIC
        ${AERON_UTILS_DIR}
        ${UNITS_DIR}
)
#target_include_directories(
#        GateLogic
#        PRIVATE
#        ${AERON_UTILS_DIR}
#        ${UNITS_DIR}
#)
target_include_directories(
        WebsocketsInterface
        INTERFACE
        ${WEBSOCKETS_DIR}/include
)
target_include_directories(
        ${PROJECT_NAME}
        PRIVATE
        ${SRC_DIR}
)


target_link_libraries(
        BinaCpp
        PRIVATE
        WebsocketsInterface
        ${WEBSOCKETS_LIB}
        pthread
        curl
        JsonCPP
        OpenSSL::SSL
)
target_link_libraries(
        JsonCPP
        INTERFACE
        jsoncpp
)
target_link_libraries(
        Aeron
        PUBLIC
        ${AERON_CLIENT_LIB}
)
target_link_libraries(
        GateLogic
        PRIVATE
        Aeron
        BinaCpp
        JsonCPP
        ${CMAKE_THREAD_LIBS_INIT}
        ${AERON_CLIENT_LIB}
)
target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        GateLogic
        WebsocketsInterface
        Aeron
        BinaCpp
        JsonCPP
)

set_target_properties(BinaCpp WebsocketsInterface PROPERTIES LINKER_LANGUAGE CXX)
