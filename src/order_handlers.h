#pragma once
#include <string>
#include <map>
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>

#include "json/json.h"

// Следующие define нужны для того, чтобы можно было использовать
// приватное поле класса BinaCPP
#define class struct
#include "binacpp.h"
#undef class
#include "../libs/aeron_cpp/src/Publisher.h"
#include "../libs/aeron_cpp/src/Subscriber.h"
#include "global_config.h"
#include "aeron_connectors.h"
#include "utils.h"

// отправка ошибок в ядро
int send_error_to_core(int code, const std::string& point, const std::string& desc);

// проверка, содержит ли json текст об ошибке
bool is_error_json(const Json::Value& json_value);


Json::Value send_order(Json::Value core_order);