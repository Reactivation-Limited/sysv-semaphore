cmake_minimum_required(VERSION 3.10)
project(SemaphoreVTests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch and build GTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Add the source directory and node-addon-api to include paths
include_directories(
    ../src
    ../src-vendor/errnoname
)

add_library(mocksys SHARED
    ../src/mock_syscalls.c
    ../src-vendor/errnoname/errnoname.c
)

# Add the test executable with all required source files
add_executable(semaphore_tests 
    ../src/semaphore-sysv.test.cpp
    ../src/semaphore-sysv.cpp
    ../src/token.cpp
    ../src-vendor/errnoname/errnoname.c
)

# Link against GoogleTest and the required libraries
target_link_libraries(semaphore_tests
    GTest::gtest
    GTest::gtest_main
    pthread
    mocksys
)

add_custom_target(build_all ALL
    DEPENDS mocksys semaphore_tests
)
