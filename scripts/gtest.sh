#!/bin/bash -e

PLATFORM=$(uname);

# Create build directory if it doesn't exist
mkdir -p "gtest/$PLATFORM"

(
    # Build and run the tests
    cd "gtest/$PLATFORM"
    # clang -dynamiclib -o mock_syscalls.dylib ../../mock_syscalls.c
    cmake ..
    make build_all
    
    case "$PLATFORM" in
        Darwin) 
          DYLD_INSERT_LIBRARIES=./libmocksys.dylib DYLD_FORCE_FLAT_NAMESPACE=1 ./semaphore_tests;;
        Linux) 
          LD_PRELOAD=./libmocksys.so ./semaphore_tests;;
    esac
)
