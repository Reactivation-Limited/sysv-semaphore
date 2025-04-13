#!/bin/bash

# Create build directory if it doesn't exist
mkdir -p tests/gtest/build

# Build and run the tests
cd tests/gtest/build
cmake ..
make
make run_tests 