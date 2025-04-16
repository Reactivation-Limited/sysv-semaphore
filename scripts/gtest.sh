#!/bin/bash -ex

PLATFORM=$(uname);

# Create build directory if it doesn't exist
mkdir -p "gtest/$PLATFORM"

(
    set -ex
    # Build and run the tests
    cd "gtest/$PLATFORM"
    cmake ..
    make build_all

    case "$PLATFORM" in
        Darwin) 
          DYLD_INSERT_LIBRARIES=./libmocksys.dylib DYLD_FORCE_FLAT_NAMESPACE=1 ./mock_syscalls_tests
          DYLD_INSERT_LIBRARIES=./libmocksys.dylib DYLD_FORCE_FLAT_NAMESPACE=1 ./semaphore_tests
          ;;
        Linux) 
          LD_PRELOAD=./libmocksys.so ./mock_syscalls_tests
          LD_PRELOAD=./libmocksys.so ./semaphore_tests
          ;;
    esac
)
