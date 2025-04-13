#!/bin/bash

# Create vendor directory if it doesn't exist
mkdir -p vendor

# Clone GoogleTest if it doesn't exist
if [ ! -d "vendor/googletest" ]; then
    git clone https://github.com/google/googletest.git vendor/googletest
fi

# Build and install GoogleTest
(
    cd vendor/googletest
    mkdir -p build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=../../.. ..
    make
    make install
)
