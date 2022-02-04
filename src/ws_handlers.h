#pragma once

#include "json/json.h"

#include "binacpp.h"
#include "utils.h"
#include "aeron_connectors.h"
#include "global_config.h"

// обработка веб-сокета Individual Symbol Ticker Stream (стакан)
int ws_depth_to_core(Json::Value &json_result);

// обработка веб-сокета User Data Stream (баланс пользователя)
int ws_balance_to_core(Json::Value &json_result);

// for DEBUG: печать json на экран
int ws_print_json( Json::Value &json_result );
