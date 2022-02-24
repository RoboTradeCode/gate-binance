#include "utils.h"

// получение текущего unix timestamp в милисекундах
long get_unix_timestamp() {
    auto now_ms = std::chrono::high_resolution_clock::now();
    auto epoch =now_ms.time_since_epoch();
    long value_ms = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
    return value_ms;
}

// функция для парсинга json
Json::Value read_raw_json(const std::string& raw_json) {
    Json::Value json_value;
    Json::Reader reader;
    reader.parse(raw_json, json_value);
    return json_value;
}

// функция для парсинга json
Json::Value read_raw_json(std::string_view raw_json) {
    Json::Value json_value;
    Json::Reader reader;
    reader.parse(std::string(raw_json), json_value);
    return json_value;
}