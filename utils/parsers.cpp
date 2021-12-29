#include "parsers.h"

/**
 * Преобразовать строку в JSON
 *
 * @param message Сообщение
 * @return        JSON
 */
Json::Value json(const std::string& message)
{
    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();
    Json::Value json;
    std::string errors;
    reader->parse(message.c_str(),message.c_str() + message.size(),&json,&errors);

    return json;
}
