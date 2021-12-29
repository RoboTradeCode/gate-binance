#ifndef BINANCE_CONNECTOR_CPP_CONVERTERS_H
#define BINANCE_CONNECTOR_CPP_CONVERTERS_H

#include <string>
#include <iomanip>
#include <sstream>

/**
 * Округлить число вниз с заданной точностью
 *
 * @param n         Число для округления
 * @param precision Точность (количество знаков после запятой)
 * @return          Строковое представление округлённого числа
 */
std::string round(double n, int precision);

#endif //BINANCE_CONNECTOR_CPP_CONVERTERS_H
