#!/bin/bash

set -e

cmake -S . -B build
cmake --build build --parallel 2

ctest --test-dir build --output-on-failure
