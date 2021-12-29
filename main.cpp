#include <csignal>
#include <jsoncpp/json/json.h>
#include "utils/Subscriber.h"
#include "utils/Publisher.h"
#include "utils/parsers.h"
#include "utils/converters.h"
#include "easylogging++.h"

// Путь до конфигурации Easylogging++
#define EASYLOGGINGPP_CONF "/home/ubuntu/binance_connector_cpp_core/easyloggingpp.conf"

// Каналы Aeron и идентификаторы их потоков
#define BALANCE_CHANNEL "aeron:ipc"
#define BALANCE_STREAM_ID 101
#define ORDER_BOOK_CHANNEL "aeron:ipc"
#define ORDER_BOOK_STREAM_ID 100
#define LOGS_CHANNEL "aeron:ipc"
#define LOGS_STREAM_ID 102
#define GATEWAY_CHANNEL "aeron:ipc"
#define GATEWAY_STREAM_ID 103

// Задержка в мс для стратегии ожидания SleepingIdleStrategy
#define IDLE_STRATEGY_DURATION 1

INITIALIZE_EASYLOGGINGPP  // NOLINT(cert-err58-cpp)

// Каналы для передачи данных по протоколу Aeron
std::shared_ptr<Subscriber> balance;
std::shared_ptr<Subscriber> order_book;
std::shared_ptr<Subscriber> logs;
std::shared_ptr<Publisher> gateway;

// Стратегия ожидания для протокола Aeron
std::shared_ptr<aeron::SleepingIdleStrategy> idle_strategy;

// Актуальный баланс
std::atomic<double> btc_balance(0);
std::atomic<double> usdt_balance(0);

// Актуальные границы удержания ордера
std::atomic<double> min_ask(0);
std::atomic<double> max_ask(0);
std::atomic<double> min_bid(0);
std::atomic<double> max_bid(0);

// Флаги наличия ордеров
std::atomic<bool> has_ask_orders(true);
std::atomic<bool> has_bid_orders(true);

/**
 * Отправить ордер в шлюз
 *
 * @param j     Ордербук (исключительно для логирования)
 * @param order Ордер
 */
void _send_order(Json::Value j, const Json::Value& order)
{
    Json::StreamWriterBuilder builder;
    const std::string message = Json::writeString(builder, order);

    VLOG(1) << message;
    LOG(INFO) << std::fixed << j["T"] << "\t" << j["s"] << "\t" << j["b"] << "\t" << min_bid << "\t" << max_bid << "\t"
              << j["a"] << "\t" << min_ask << "\t" << max_ask << "\t" << btc_balance << "\t" << usdt_balance << "\t"
              << order["a"] << "\t" << order["s"] << "\t" << "" << "\t" << "";

    gateway->offer(message);
}

/**
 * Создать ордер
 *
 * @param j        Ордербук (исключительно для логирования)
 * @param side     Тип ордера ("SELL", "BUY")
 * @param price    Цена
 * @param quantity Объём
 */
void create_order(Json::Value j, const std::string& side, double price, double quantity)
{
    Json::Value order;
    order["a"] = "+";                 // action
    order["S"] = "BTCUSDT";           // symbol
    order["s"] = side;                // side
    order["t"] = "LIMIT";             // type
    order["p"] = round(price, 2);     // price
    order["q"] = round(quantity, 5);  // quantity

    if (side == "SELL")
        has_ask_orders = true;
    else if (side == "BUY")
        has_bid_orders = true;

    _send_order(std::move(j), order);
}

/**
 * Отменить ордер
 *
 * @param j    Ордербук (исключительно для логирования)
 * @param side Тип ордера ("SELL", "BUY")
 */
void cancel_order(Json::Value j, const std::string& side)
{
    Json::Value order;
    order["a"] = "-";        // action
    order["S"] = "BTCUSDT";  // symbol
    order["s"] = side;       // side

    if (side == "SELL")
        has_ask_orders = false;
    else if (side == "BUY")
        has_bid_orders = false;

    send_order(std::move(j), order);
}

/**
 * Функция обратного вызова Aeron для приёма информации о балансе
 */
void balance_handler(const aeron::AtomicBuffer& buffer,
        aeron::util::index_t offset,
        aeron::util::index_t length,
        const aeron::Header& header)
{
    // Получить сообщение из буфера Aeron, преобразовать в JSON
    const char* s = reinterpret_cast<const char*>(buffer.buffer()) + offset;
    auto n = static_cast<std::size_t>(length);
    std::string message(s, n);
    Json::Value j = json(message);
    VLOG(1) << j;

    // Перебрать монеты и обновить баланс
    for (auto symbol : j["B"])
    {
        if (symbol["a"] == "BTC")
            btc_balance = std::stod(symbol["f"].asString());

        else if (symbol["a"] == "USDT")
            usdt_balance = std::stod(symbol["f"].asString());
    }
}

/**
 * Функция обратного вызова Aeron для приёма ордербуков
 */
void order_book_handler(const aeron::AtomicBuffer& buffer,
        aeron::util::index_t offset,
        aeron::util::index_t length,
        const aeron::Header& header)
{
    // Получить сообщение из буфера Aeron, преобразовать в JSON
    const char* s = reinterpret_cast<const char*>(buffer.buffer()) + offset;
    auto n = static_cast<std::size_t>(length);
    std::string message(s, n);
    Json::Value j = json(message);

    // Рассчитать цену и объём возможного ордера
    double best_ask = std::stod(j["a"].asString());
    double best_bid = std::stod(j["b"].asString());
    double price_ask = best_ask * 1.0015;
    double price_bid = best_bid * 0.9985;
    double ask_order_amount = btc_balance;
    double bid_order_amount = usdt_balance / price_ask;

    // Если есть ASK-ордер, но лучшее предложение за пределами удержания — отменить ASK-ордер
    if (has_ask_orders && !(min_ask < best_ask && best_ask < max_ask))
    {
        cancel_order(j, "SELL");
    }

    // Если есть BID-ордер, но лучшее предложение за пределами удержания — отменить BID-ордер
    if (has_bid_orders && !(min_bid < best_bid && best_bid < max_bid))
    {
        cancel_order(j, "BUY");
    }

    // Если нет ASK-ордера и есть BTC — создать ASK-ордер
    if (!has_ask_orders && btc_balance > 0.0008)
    {
        // Рассчитать границы удержания ASK-ордера
        min_ask = best_ask * 0.9995;
        max_ask = best_ask * 1.0005;

        create_order(j, "SELL", price_ask, ask_order_amount);
    }

    // Если нет BID-ордера и есть USDT — создать BID-ордер
    if (!has_bid_orders && usdt_balance > 40)
    {
        // Рассчитать границы удержания BID-ордера
        min_bid = best_bid * 0.9995;
        max_bid = best_bid * 1.0005;

        create_order(j, "BUY", price_bid, bid_order_amount);
    }
}

/**
 * Функция обратного вызова Aeron для приёма логов
 */
void logs_handler(const aeron::AtomicBuffer& buffer,
        aeron::util::index_t offset,
        aeron::util::index_t length,
        const aeron::Header& header)
{
    // Получить сообщение из буфера Aeron, преобразовать в JSON
    const char* s = reinterpret_cast<const char*>(buffer.buffer()) + offset;
    auto n = static_cast<std::size_t>(length);
    std::string message(s, n);
    Json::Value event = json(message);
    LOG(ERROR) << event;
}

// Состояние работы программы. Изменения значения на false приводит к остановке программы
std::atomic<bool> running(true);

/**
 * Функция обратного вызова для обработки прерывания <Ctrl+C>. Приводит к остановке программы
 */
void sigint_handler(int)
{
    running = false;
}

int main(int argc, char* argv[])
{
    // Конфигурация Easylogging++
    START_EASYLOGGINGPP(argc, argv);
    el::Configurations conf(EASYLOGGINGPP_CONF);
    el::Loggers::reconfigureAllLoggers(conf);

    // Инициализация каналов для передачи данных по протоколу Aeron.
    // Каждый канал связан с функцией обратного вызова, которая обрабатывает поступающие в него данные
    balance = std::make_shared<Subscriber>(balance_handler, BALANCE_CHANNEL, BALANCE_STREAM_ID);
    order_book = std::make_shared<Subscriber>(order_book_handler, ORDER_BOOK_CHANNEL, ORDER_BOOK_STREAM_ID);
    logs = std::make_shared<Subscriber>(logs_handler, LOGS_CHANNEL, LOGS_STREAM_ID);
    gateway = std::make_shared<Publisher>(GATEWAY_CHANNEL, GATEWAY_STREAM_ID);

    // Инициализация стратегии ожидания для протокола Aeron
    idle_strategy = std::make_shared<aeron::SleepingIdleStrategy>(std::chrono::duration<long, std::milli>(IDLE_STRATEGY_DURATION));

    // Подключение к каналам для передачи данных по протоколу Aeron
    order_book->connect();
    balance->connect();
    logs->connect();
    gateway->connect();

    // Главный цикл. Опрашивает каналы на наличие новых сообщений.
    // Останавливается прерыванием <Ctrl+C>
    signal(SIGINT, sigint_handler);
    while (running)
    {
        int fragments_read_balance = balance->poll();
        int fragments_read_order_book = order_book->poll();
        int fragments_read_logs = logs->poll();

        // Выполнение стратегии ожидания — пауза в 1мс при простое
        int fragments_read = fragments_read_balance + fragments_read_order_book + fragments_read_logs;
        idle_strategy->idle(fragments_read);
    }
}
