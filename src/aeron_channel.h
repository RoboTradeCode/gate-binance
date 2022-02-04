#pragma once
#include <iostream>

/// Структура описывающая канал aeron
struct aeron_channel{
    // "aeron:udp?endpoint=54.248.171.18:20121"
    // "aeron:udp?control=172.31.14.205:40456|control-mode=dynamic"
    // "aeron:ipc"
    std::string channel;
    // идентификатор потока
    int stream_id = 1001;
};