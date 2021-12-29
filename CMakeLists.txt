cmake_minimum_required(VERSION 3.22)
project(binance_connector_cpp)

# Aeron
find_package(Threads REQUIRED)
include_directories(/home/ubuntu/aeron/aeron-client/src/main/cpp)

# JsonCpp
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
link_libraries(${JSONCPP_LIBRARIES})

# Easylogging++
include_directories(easyloggingpp/src)

add_executable(Core
        main.cpp
        utils/Subscriber.cpp utils/Subscriber.h
        utils/Publisher.cpp utils/Publisher.h
        utils/parsers.cpp utils/parsers.h
        easyloggingpp/src/easylogging++.cc easyloggingpp/src/easylogging++.h
        utils/converters.cpp utils/converters.h
)

target_link_libraries(Core PRIVATE
        libaeron_client.a
        Threads::Threads
        ${JSONCPP_LIBRARIES}
)
