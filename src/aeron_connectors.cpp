#include "aeron_connectors.h"

shared_ptr<Publisher> depth_publisher;
shared_ptr<Publisher> balance_publisher;
shared_ptr<Publisher> errors_publisher;

shared_ptr<Subscriber> order_subscriber;
// обработчик ордеров от ядра
// нужен для Subscriber, принимающего ордера от ядра
void handle_core_order(string_view order_message) {
    std::cout << order_message << std::endl;
    // преобразую строку ордера в json
    Json::Value core_order = read_raw_json(order_message);

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
}