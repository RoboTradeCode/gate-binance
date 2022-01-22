//
// Created by qod on 23.12.2021.
//

#include "ws_handlers.h"


// обработка веб-сокета Individual Symbol Ticker Stream (стакан)
int ws_depth_to_core(Json::Value &json_result) {
    /* Получение Unix timestamp, добавление его в json
     * и отправка через aeron ядру
     */
    // получаю unix timestamp
    // конвертирую в double, т.к. в json не получается добавить поле типа long
    auto timestamp = static_cast<double>(get_unix_timestamp());

    // добавляю в json поле для unix timestamp
    json_result["u"] = timestamp;

    // конвертирую json::value в строку
    std::string message = json_result.toStyledString();

    // Отправляю сообщение
    auto result = get_depth_publisher().offer(message);
    return 0;
}

// обработка веб-сокета User Data Stream (баланс пользователя)
int ws_balance_to_core(Json::Value &json_result) {
    // Веб-сокет возвращает несколько сообщений о данных пользователя
    // Но меня интересуют только сообщения, касающие баланса
    if (json_result["e"] == "outboundAccountPosition") {
        // конвертирую json::value в строку
        std::string message = json_result.toStyledString();

        // Отправляю сообщение по aeron в ядро
        get_balance_publisher().offer(message);

        std::cout << json_result << std::endl;
    }
    return 0;
}

// for DEBUG: печать json на экран
int ws_print_json( Json::Value &json_result ) {
    std::cout << json_result.toStyledString() << std::endl;
    return 0;
}