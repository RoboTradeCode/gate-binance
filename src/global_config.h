// Используется для создания конфигурации, которая используется во всем шлюзе
// Базовые настройки используются только для инициализации в точке входа, но
// название биржи и ключи нужны в других частях шлюза

#pragma once
#include <string>
#include "gate_config.h"


extern shared_ptr<gate_config> config;

