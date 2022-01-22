#include "aeron_connectors.h"

// получение Publisher, в который будут отправляться данные эндпоинта
// Individual Symbol Ticker Stream (стакан)
Publisher get_depth_publisher() {
    static Publisher depth_publisher = Publisher("aeron:ipc", 100);
    static int flag = -1;
    if (flag == -1) {
        flag = depth_publisher.connect();
    }
    return depth_publisher;
}

// получение Publisher, в который будут отправляться данные эндпоинта
// User Data Stream (баланс пользователя)
Publisher get_balance_publisher() {
    static Publisher balance_publisher = Publisher("aeron:ipc", 101);
    static int flag = -1;
    if (flag == -1) {
        flag = balance_publisher.connect();
    }
    return balance_publisher;
}

// получение Publisher, в который будут отправляться ошибки
Publisher get_errors_publisher() {
    static Publisher errors_publisher = Publisher("aeron:ipc", 102);
    static int flag = -1;
    if (flag == -1) {
        flag = errors_publisher.connect();
    }
    return errors_publisher;
}

// получение Subscriber, в который ядро передает ордера
Subscriber get_order_subscriber() {
    static Subscriber order_subscriber = Subscriber(&handle_core_order, "aeron:ipc", 103);
    static int flag = -1;
    if (flag == -1) {
        flag = order_subscriber.connect();
    }
    return order_subscriber;
}


// обработчик ордеров от ядра
// нужен для Subscriber, принимающего ордера от ядра
aeron::fragment_handler_t handle_core_order() {
    // значения, которые должен возвращать handler класса aeron Subscriber
    return [&](const aeron::AtomicBuffer& buffer, aeron::util::index_t offset, aeron::util::index_t length,
               const aeron::Header& header){
        // логика обработчика

        // получаю ордер из aeron в виде строки
        std::string received_order = std::string(reinterpret_cast<const char *>(buffer.buffer()) + offset,
                                                 static_cast<std::size_t>(length));

        std::cout << received_order << std::endl;
        // преобразую строку ордера в json
        Json::Value core_order = read_raw_json(received_order);

        // отправляю ордер в binance API
        Json::Value result = send_order(core_order);
        // проверяю, вернул ли binance ошибку
        std::cout << result.toStyledString() << std::endl;
        if (is_error_json(result)) {
            send_error_to_core(
                    result["code"].asInt(),
                    "c",
                    result["msg"].asString()
            );
        }
    };
}