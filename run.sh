#!/usr/bin/env bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j$(nproc) && time ./build/bordeaux tests/main.bdx
