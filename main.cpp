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

#include "Publisher.h"
#include "Subscriber.h"

#include "config.h"
#include "order_handlers.h"
#include "aeron_connectors.h"
#include "ws_handlers.h"

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
        // получаю subscriber, отвечающий за получение ордеров
        Subscriber order_subscriber = get_order_subscriber();

        /* 1 */
        // Получение непрочитанных сообщений об ордерах из Aeron и их
        // обработка, если они имеются. Gateway формирует ордер, отправляет
        // его в Binance API, и обрабатывает ответ. Если Binance сообщает
        // о неправильном ордере, отправляет сообщение ядру через канал Aeron.
        order_subscriber.poll();

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

    // инициализация API ключей Binance, нужных для получения данных
    // и выставления ордеров
    // информацию по балансу пользователя.
    std::string api_key = API_KEY;
    std::string secret_key = SECRET_KEY;

    // инициализация классов для работы с Binance API
    BinaCPP::init(api_key, secret_key);
    BinaCPP_websocket::init();


    // Подключение к каналам Aeron. Всего четыре канала:
    // 1. Канал отправки данных стакана
    // 2. Канал отправки данных баланса пользователя
    // 3. Канал отправки ошибок
    // 4. Канал получения команд на управление ордерами от ядра (создание и отмена)

    // массив Publisher, к которым нужно подключиться
    // они нужны для отправки данных ядру
    Publisher publisher_connection_targets[] {
            get_depth_publisher(),   // отправка стакана
            get_balance_publisher(), // отправка баланса
            get_errors_publisher()   // отправка ошибок
    };

    // массив Subscriber, к которым нужно подключиться
    Subscriber subscriber_connection_targets[] {
            get_order_subscriber()  // получение ордеров
    };

    // попытка подключения всех Publisher и Subscriber к Aeron Media Driver
    int connection = -1;
    while (connection != 0) {
        connection = 0;
        // при удачной попытке метод connect() возвращает 0
        // при неудачной попытке метод connect() возвращает -1
        for (auto it : publisher_connection_targets) {
            connection += it.connect();
        }
        for (auto it : subscriber_connection_targets) {
            connection += it.connect();
        }
        // при неудачной попытке сплю десять секунд,
        // чтобы не спамить подключениями
        if (connection != 0)
            sleep(10);
    }


    // Подключение веб-сокетов Binance, для получения данных по стакану и
    // данных о балансе пользователя. Это позволяет не
    // нагружать Binance API постоянными запросами.
    connect_ws();

    // запуск главного цикла
    gateway_loop();

    return 0;
}