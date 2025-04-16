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

# Build mock_syscalls as a shared library
add_library(mocksys SHARED
    ../src/mock_syscalls.cpp
    ../src-vendor/errnoname/errnoname.c
)

# Add GTest include directory to mocksys target
target_include_directories(mocksys PUBLIC ${googletest_SOURCE_DIR}/googletest/include)

# Create mock_syscalls test executable
add_executable(mock_syscalls_tests
    ../src/mock_syscalls.test.cpp
)

# Link against GoogleTest and mocksys library
target_link_libraries(mock_syscalls_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    mocksys
    pthread
)

# Add the semaphore test executable
add_executable(semaphore_tests 
    ../src/semaphore-sysv.test.cpp
    ../src/semaphore-sysv.cpp
    ../src/token.cpp
    ../src-vendor/errnoname/errnoname.c
)

# Link against GoogleTest and the required libraries
target_link_libraries(semaphore_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    mocksys
    pthread
)

add_custom_target(build_all ALL
    DEPENDS mocksys mock_syscalls_tests semaphore_tests
)
