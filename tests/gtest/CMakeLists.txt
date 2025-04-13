cmake_minimum_required(VERSION 3.10)
project(SemaphoreVTests)

# Find GoogleTest package
find_package(GTest REQUIRED)

# Add the source directory and node-addon-api to include paths
include_directories(
    ../../src
    ../../src-vendor/errnoname
)

# Add the test executable with all required source files
add_executable(semaphore_tests 
    ../../src/semaphore-sysv.test.cpp
    ../../src/semaphore-sysv.cpp
    ../../src/token.cpp
    ../../src-vendor/errnoname/errnoname.c
)

# Link against GoogleTest and the required libraries
target_link_libraries(semaphore_tests
    GTest::GTest
    GTest::Main
    pthread
)

# Add a custom target to run the tests
add_custom_target(run_tests
    COMMAND semaphore_tests
    DEPENDS semaphore_tests
) 