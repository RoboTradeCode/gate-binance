#!/bin/sh

# Сборка шлюза
echo "Building Binance Gateway..."
mkdir -p build/Debug
cd build/Debug || exit 1
cmake ../..
cmake --build .
