#include <string>
#include <chrono>

#include "binacpp.h"

// Следующие define нужны для того, чтобы можно было использовать
// приватное поле класса BinaCPP_websocket.
// Это нужно, чтобы можно было вручную обрабатывать
// получение информации от веб-сокетов binance_websockets
// у struct поля по умолчанию публичные, у class приватные
#define class struct
#include "binacpp_websocket.h"
#undef class

#include "../libs/aeron_cpp/src/Publisher.h"
#include "../libs/aeron_cpp/src/Subscriber.h"

#include "order_handlers.h"
#include "aeron_connectors.h"
#include "ws_handlers.h"

shared_ptr<gate_config> config;

// получает баланс пользователя, отправляет в ядро по aeron
void send_balances_to_core() {
    // получает данные аккаунта пользователя
    // после этого отсеивает токены с нулевым балансом и заполняет нужные поля


    long recvWindow = 10000;
    Json::Value account;
    Json::Value json_balances;

    // вектор для хранения балансов по токенам
    vector <Json::Value> balances;

    // получаю данные аккаунта пользователя
    BinaCPP::get_account(recvWindow, account);

    // перебираю балансы по токенам (почти все из них нулевые)
    for (int i = 0; i < account["balances"].size(); i++) {
        // получаю баланс по конкретному токену
        auto balance = account["balances"][i];
        // проверяю, является ли баланс НЕ нулевым
        if (stof(balance["free"].asCString()) != 0 or
            stof(balance["locked"].asCString()) != 0) {
            // баланс больше нуля, добавляю в вектор с балансами
            balances.push_back(balance);
        }
    }

    // после этого нужно перенести в json, которые соответствует принятому формату

    // переношу балансы в json
    int i = 0;
    for (const auto& bal : balances) {
        json_balances["B"][i]["a"] = bal["asset"];
        json_balances["B"][i]["f"] = bal["free"];
        json_balances["B"][i]["l"] = bal["locked"];
        i++;
    }
    // тип события - такое событие соответствует обновлению балансу пользователя
    // в этом случае это неправда - баланс не обновился. Но с этим полем можно использовать
    // обработчик ws_balance_to_core(), не создавать отдельную функцию
    json_balances["e"] = "outboundAccountPosition";
    // время последнего обновления
    json_balances["u"] = account["updateTime"];
    // текущее время
    json_balances["E"] = static_cast<double>(get_unix_timestamp());

    // отправляю полученный json в ядро по aeron
    ws_balance_to_core(json_balances);
}

// Запрашиваю у биржи ключ user data для получения баланса
// ключ нужно будет запрашивать снова, иначе он истечет через 60 минут
string get_listenKey() {
    // инициализация Json, в него BinaCPP поместит ответ от Binance API
    Json::Value result;

    // Запрашиваю ключ у Binance API
    BinaCPP::start_userDataStream(result);

    return result["listenKey"].asString();
}
// Подключение веб-сокета Individual Symbol Ticker Stream (стакан)
void connect_depth() {
    BinaCPP_websocket::connect_endpoint(ws_depth_to_core, "/ws/btcusdt@bookTicker");
}

// Подключение веб-сокета User Data Stream
void connect_user_data() {
    // Получаю ключ для получения данных пользователя
    string listenKey = get_listenKey();
    // формирую эндпоинт
    string user_data_path = "/ws/" + listenKey;
    // подключаюсь к эндпоинту
    BinaCPP_websocket::connect_endpoint(ws_balance_to_core, user_data_path.c_str());
}

// подключение веб-сокетов, данные из них передаются в обработчики
void connect_ws() {
    connect_depth();
    connect_user_data();
}


// Эта функция циклично обрабатывает полученные данные и команды от ядра
// Также она проверяет, сколько времени прошло с открытия веб-сокета - нужно ли его переподключить
[[noreturn]] void gateway_loop() {
    // время, когда был открыт веб-сокет, чтобы понимать, когда его нужно переподключить
    auto depth_connect_time = std::chrono::high_resolution_clock::now();
    auto user_data_connect_time = std::chrono::high_resolution_clock::now();


    /* Алгоритм работы цикла:
     1. Получение непрочитанных сообщений об ордерах из Aeron и их
     обработка, если они имеются. Gateway формирует ордер, отправляет
     его в Binance API, и обрабатывает ответ. Если Binance сообщает
     о неправильном ордере, отправляет сообщение ядру через канал Aeron.
     2. Обработка данных, которые получают веб-сокеты. Данные по стакану
     и изменениям баланса пользователя отправляются ядру через Aeron.
     3. Происходит проверка времени работы веб сокетов. Если прошло
     половина времени их работы, происходит переподключение. Если этого
     не сделать, то поток данных прервется через некоторое время.
     */
    while (true) {
        /* 1 */
        // Получение непрочитанных сообщений об ордерах из Aeron и их
        // обработка, если они имеются. Gateway формирует ордер, отправляет
        // его в Binance API, и обрабатывает ответ. Если Binance сообщает
        // о неправильном ордере, отправляет сообщение ядру через канал Aeron.
        order_subscriber->poll();

        /* 2 */
        // Обработка данных, которые получают веб-сокеты. Данные по стакану
        // и изменениям баланса пользователя отправляются ядру через Aeron.
        lws_service( BinaCPP_websocket::context, 500 );

        /* 3 */
        // Происходит проверка времени работы веб сокетов. Если прошло
        // половина времени их работы, происходит переподключение. Если этого
        // не сделать, то поток данных прервется через некоторое время.

        // веб-сокет User Data Stream (баланс пользователя)
        if (std::chrono::duration_cast<std::chrono::minutes>(
                std::chrono::high_resolution_clock::now() - user_data_connect_time) >= 30min
                ) {
            // Для переподключения User Data Stream достаточно отправить запрос на получение
            // ключа. При этом сам ключ останется прежним, но его срок действия продлится
            // продление ключа.
            get_listenKey();
            // обновление времени последнего переподключения
            user_data_connect_time = std::chrono::high_resolution_clock::now();
        }
        // веб-сокет Individual Symbol Ticker Stream (стакан)
        if (std::chrono::duration_cast<std::chrono::hours>(
                std::chrono::high_resolution_clock::now() - depth_connect_time) >= 12h
                ) {
            // переподключение веб-сокета стакана
            connect_depth();
            // обновление времени последнего переподключения
            depth_connect_time = std::chrono::high_resolution_clock::now();
        }
    }
}

// Точка входа шлюза
int main() {
    /* Выполняемые действия при запуске:
     * - Инициализация API ключей Binance
     * - Инициализация классов для работы с Binance API
     * - Инициализация классов для работы с Aeron
     * - Подключение к Aeron
     * - Подключение веб-сокетов
     * - Запуск главного цикла
     */

    // загрузка конфигурации шлза
    config = make_shared<gate_config>("config.toml");


    // инициализация классов для работы с Binance API
    BinaCPP::init(config->account.api_key, config->account.secret_key);
    BinaCPP_websocket::init();

    // Подключение к каналам Aeron. Всего четыре канала:
    // 1. Канал отправки данных стакана
    // 2. Канал отправки данных баланса пользователя
    // 3. Канал отправки ошибок
    // 4. Канал получения команд на управление ордерами от ядра (создание и отмена)

    // Publisher's, к которым нужно подключиться
    // они нужны для отправки данных ядру
    // отправка стакана
    depth_publisher = std::make_shared<Publisher>(config->aeron.publishers.orderbook.channel,
                                                  config->aeron.publishers.orderbook.stream_id);
    // отправка баланса
    balance_publisher = std::make_shared<Publisher>(config->aeron.publishers.balance.channel,
                                                    config->aeron.publishers.balance.stream_id);
    // отправка ошибок
    errors_publisher = make_shared<Publisher>(config->aeron.publishers.logs.channel,
                                              config->aeron.publishers.logs.stream_id);

    //Subscriber, к которому нужно подключиться
    // получение ордеров
    order_subscriber = std::make_shared<Subscriber>(handle_core_order, config->aeron.subscribers.core.channel,
                                                    config->aeron.subscribers.core.stream_id);


    //  отправляю текущий баланс в ядро
    send_balances_to_core();

    // Подключение веб-сокетов Binance, для получения данных по стакану и
    // данных о балансе пользователя. Это позволяет не
    // нагружать Binance API постоянными запросами.
    connect_ws();

    // запуск главного цикла
    gateway_loop();

    return 0;
}