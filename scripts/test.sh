#!/bin/bash

set -e

cmake -S . -B build
cmake --build build --parallel 3

ctest --test-dir build --output-on-failure
