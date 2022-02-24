#include "order_handlers.h"

//// TODO: Remove when global_config is fixed
//// Hotfix by nomnoms12
//std::string api_key = "Q0LPrj20sAGqptchS8ZiR2kJzUggr5W3CZVRxIRzB8Nr1OSgbXivjF62YVE0E98e";
//std::string secret_key = "V9fu4jT21mzejk0sdq3jutubN6xxLMvBtY6ZKRJYXb3kNlYMJfoeTxXG8qDweRek";

std::string round_floor(std::string x, int n);

// отправка ошибок в ядро
int send_error_to_core(int code, const std::string& point, const std::string& desc = "") {
    /* Формирует Json ошибки и отправляет его в ядро через aeron
     * аргументы:
     * int code : код ошибки
     * const string& point : в какой точке произошла ошибка (g - gate, c - core)
     * const std::string& desc : описание ошибки (может отсутствовать)
     */
    Json::Value error_json;
    error_json["c"] = code;
    error_json["p"] = point;
    if (!desc.empty())
        error_json["e"] = desc;

    // получаю unix timestamp
    // конвертирую в double, т.к. в json не получается добавить поле типа long
    auto timestamp = static_cast<double>(get_unix_timestamp());

    // добавляю в json поле для unix timestamp
    error_json["t"] = timestamp;

    std::string message = error_json.toStyledString();

    // Отправляю сообщение
    errors_publisher->offer(message);

    return 0;
}

// проверка, содержит ли json текст об ошибке
bool is_error_json(const Json::Value& json_value) {
    // json с текстом ошибки должен содержать поля msg и code
    return json_value.isMember("msg") and json_value.isMember("code");
}

// функция для обработки и отправки ордера от ядра в binance
Json::Value send_order(Json::Value core_order) {

    // здесь повторная инициализация класса библиотеки Binance С++
    // это нужно, иначе в непрерывной сессии BinCPP могут начаться ошибки
    BinaCPP::cleanup();
    BinaCPP::init(config->account.api_key, config->account.secret_key);

    // json для результата отправки ордера
    Json::Value result;
    // окно запроса - нужно для ожидания ответа от binance
    long recvWindow = 10000;

    // обработка выставления ордера
    if (core_order["a"] == "+") {
        BinaCPP::send_order(
                core_order["S"] == "BTC-USDT" ? "BTCUSDT" : core_order["S"].asCString(),
                core_order["s"].asCString(),
                core_order["t"].asCString(),
                "GTC",
                std::stod(round_floor(core_order["q"].asCString(), 5)),
                std::stod(round_floor(core_order["p"].asCString(), 2)),
                "",
                0,
                0,
                recvWindow,
                result
        );
    } // обработка отмены ордера
    else if (core_order["a"] == "-") {
        // получение открытых ордеров
        Json::Value open_orders;

        BinaCPP::get_openOrders(
                core_order["S"] == "BTC-USDT" ? "BTCUSDT" : core_order["S"].asCString(),
                recvWindow,
                open_orders);
        std::cout << open_orders << endl;
        // получение ордера, который требуется отменить
        long current_order = 0;
        for (auto order : open_orders) {
            std::cout << order["side"].asString() << " |vs| " << core_order["s"].asString() << std::endl;
            if (order["side"].asString() == core_order["s"].asString()) {
                current_order = order["orderId"].asLargestUInt();
                break;
            }
        }

        // проверка, что ордер для отмены был найден
        if (current_order != 0) {
            std::cout << "Attempt to cancel order" << std::endl;
            BinaCPP::cleanup();
            BinaCPP::init(config->account.api_key, config->account.secret_key);
            BinaCPP::cancel_order(
                    core_order["S"] == "BTC-USDT" ? "BTCUSDT" : core_order["S"].asCString(),
                    current_order,
                    "",
                    "", recvWindow,
                    result
            );
        }
    }
    return result;
}

/** Округлить строковое представление числа в меньшую сторону до n знаков после запятой
 *
 * @param x Строковое представление числа
 * @param n Необходимое количество знаков после запятой
 * @return Округлённое число
 */
std::string round_floor(std::string x, int n)
{
    return x.substr(0, x.find('.') + n + 1);
}
