# Зависимости и сборка Binance Gateway

> Проверено на Linux Ubuntu 20.04.3 LTS

### Зависимости Binance Gateway

1. Склонировать исходный код Gateway.

2.  Клонировать в директорию `lib` две библиотеки:

https://github.com/real-logic/aeron
https://github.com/binance-exchange/binacpp

3. Установить библиотеки:

```bash
sudo apt install libjsoncpp-dev libcurl4-openssl-dev 
```

4. Установка libssl1.0.0:

   1. Добавьте репозиторий для нужной версии библиотеки: Откройте файл `sudo nano /etc/apt/sources.list` и добавьте в конец  файла строку `deb http://security.ubuntu.com/ubuntu xenial-security main`
   
   2. Обновите список репозиториев `sudo apt update`
   
   3. Установите библиотеку `sudo apt install libssl1.0.0`
   
5. Соберите библиотеку Aeron. Для этого перейдите в папку `aeron` и выполните команду:

`./cppbuild/cppbuild`

6. *Не обязательно*. Обновить библиотеку для конфигурации Toml++, т.е.
удалить директорию `toml++`, которая находится в `lib`, и скопировать в директорию `lib`
следующую директорию из github:

https://github.com/marzer/tomlplusplus/tree/master/include/toml%2B%2B


Полный список команд примерно такой:

```bash
git clone # адрес репозиторий Gateway
cd lib
git clone https://github.com/real-logic/aeron.git
git clone https://github.com/binance-exchange/binacpp.git
sudo apt install libjsoncpp-dev libcurl4-openssl-dev 
# после открытия файла нужно ввести строку вручную
sudo nano /etc/apt/sources.list # ввести строку deb http://security.ubuntu.com/ubuntu xenial-security main
sudo apt update
sudo apt install libssl1.0.0
cd aeron
./cppbuild/cppbuild
```

### Сборка Gateway

Сборка производится с помощью CMake. Проверить, установлен ли он можно с помощью команды:

```bash	
cmake --version
```

Минимальная версия`3.0`. Чтобы установить CMake:

```bash
sudo apt install cmake
```

Чтобы собрать Gateway, выполните следующие команды:

```bash
mkdir build 
cd build 
cmake ../ 
make
```

### Запуск Gateway

Перед запуском убедитесь, что медиа-драйвер Aeron запущен. Чтобы запустить его, перейдите в папку `lib/aeron`, и выполните команду:

```bash 
./cppbuild/Release/binaries/aeronmd
```

Чтобы запуcтить, перейдите в папку `build` и выполните:
```bash 
./BinanceGateway
```
