#pragma once
#include "../libs/aeron_cpp/src/Publisher.h"
#include "../libs/aeron_cpp/src/Subscriber.h"
#include "order_handlers.h"
#include "json/json.h"

extern shared_ptr<Publisher> depth_publisher;
extern shared_ptr<Publisher> balance_publisher;
extern shared_ptr<Publisher> errors_publisher;

extern shared_ptr<Subscriber> order_subscriber;

// обработчик ордеров от ядра
// нужен для Subscriber, принимающего ордера от ядра
void handle_core_order(string_view order_message);
