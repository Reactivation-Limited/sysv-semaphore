#!/bin/bash

# Create build directory if it doesn't exist
mkdir -p tests/gtest/build

# Build and run the tests
cd tests/gtest/build

clang -dynamiclib -o mock_syscalls.dylib ../../mock_syscalls.c

cmake ..
make

DYLD_INSERT_LIBRARIES=./mock_syscalls.dylib DYLD_FORCE_FLAT_NAMESPACE=1 ./semaphore_tests
