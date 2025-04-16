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
  MockCall mock_ftok{};
  mock_ftok.syscall = MOCK_FTOK;
  mock_ftok.args.ftok_args.pathname = __FILE__;
  mock_ftok.args.ftok_args.proj_id = 42;
  mock_ftok.return_value = 1234;
  mock_ftok.errno_value = 0;
  mock_push_expected_call(mock_ftok);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  MockCall mock{};
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0666 | IPC_CREAT | IPC_EXCL;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl_args.semid = 42;
  mock.args.semctl_args.semnum = 0;
  mock.args.semctl_args.cmd = SETVAL;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  SemaphoreV *sem = SemaphoreV::create(key, 0666, 1);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}