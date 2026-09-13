#!/bin/bash

set -e

cmake -S . -B build
cmake --build build

ctest --test-dir build --output-on-failure