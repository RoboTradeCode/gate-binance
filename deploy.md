Для работы gateway нужно склонировать в папку lib две библиотеки:

https://github.com/real-logic/aeron
https://github.com/binance-exchange/binacpp

также нужно собрать библиотеку Aeron. Для этого выполнить следующую команду, находясь в папке `aeron`:

`./cppbuild/cppbuild`

Полный список команд примерно такой:

```bash
cd lib
git clone https://github.com/real-logic/aeron.git
git clone https://github.com/binance-exchange/binacpp.git
cd aeron
```
