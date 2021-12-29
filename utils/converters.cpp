#include <cfenv>
#include "converters.h"

/**
 * Округлить число вниз с заданной точностью
 *
 * @param n         Число для округления
 * @param precision Точность (количество знаков после запятой)
 * @return          Строковое представление округлённого числа
 */
std::string round(double n, int precision)
{
    std::stringstream stream;
    std::fesetround(FE_DOWNWARD);
    stream << std::fixed << std::setprecision(precision) << n;
    std::string quantity = stream.str();

    return quantity;
}
