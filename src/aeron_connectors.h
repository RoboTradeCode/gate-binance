#pragma once
#include "Publisher.h"
#include "Subscriber.h"
#include "order_handlers.h"
#include "json/json.h"


// получение Publisher, в который будут отправляться данные эндпоинта
// Individual Symbol Ticker Stream (стакан)
Publisher get_depth_publisher();

// получение Publisher, в который будут отправляться данные эндпоинта
// User Data Stream (баланс пользователя)
Publisher get_balance_publisher();

// получение Publisher, в который будут отправляться ошибки
Publisher get_errors_publisher();

// получение Subscriber, в который ядро передает ордера
Subscriber get_order_subscriber();

// обработчик ордеров от ядра
// нужен для Subscriber, принимающего ордера от ядра
aeron::fragment_handler_t handle_core_order();
