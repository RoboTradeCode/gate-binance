#ifndef AERONEXAMPLES_UTILS_H
#define AERONEXAMPLES_UTILS_H

#include "Aeron.h"

/**
 * Функция, возвращающая Callback для печати сообщений на экран
 *
 * @return Callback для печати сообщений на экран
 */
aeron::fragment_handler_t print_string_message();

#endif  // AERONEXAMPLES_UTILS_H
