#pragma once
#include <chrono>
#include <json/json.h>

// получение текущего unix timestamp в милисекундах
long get_unix_timestamp();

Json::Value read_raw_json(const std::string& raw_json);
