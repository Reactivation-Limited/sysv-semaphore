#include "semaphore-sysv.h"
#include "mock_syscalls.hpp"
#include <cerrno>
#include <errnoname.c>
#include <errnoname.h>
#include <gtest/gtest.h>
#include <sys/sem.h>

class SemaphoreVTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Reset errno before each test
    errno = 0;
    mock_reset();
  }
};

TEST_F(SemaphoreVTest, SemgetWorksWithStack) {
  MockCall mock_ftok = {.syscall = MOCK_FTOK,
                        .args.ftok_args = {.pathname = __FILE__, .proj_id = 42},
                        .return_value = 1234,
                        .errno_value = 0};
  mock_push_expected_call(mock_ftok);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  MockCall mock = {.syscall = MOCK_SEMGET,
                   .args.semget_args = {.key = key.valueOf(), .nsems = 1, .semflg = 0666},
                   .return_value = 42,
                   .errno_value = 0};
  mock_push_expected_call(mock);

  // auto semaphore = SemaphoreV::open(0x1234);

  SemaphoreV *sem = SemaphoreV::create(key, 1, 0666);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}