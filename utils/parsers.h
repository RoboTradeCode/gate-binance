#ifndef BINANCE_CONNECTOR_CPP_PARSERS_H
#define BINANCE_CONNECTOR_CPP_PARSERS_H

#include <jsoncpp/json/json.h>

/**
 * Преобразовать строку в JSON
 *
 * @param message Сообщение
 * @return        JSON
 */
Json::Value json(const std::string&);

#endif  // BINANCE_CONNECTOR_CPP_PARSERS_H
