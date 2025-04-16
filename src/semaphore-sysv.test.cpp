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

TEST_F(SemaphoreVTest, CreateExclusiveSucceeds) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL; // check the mode is masked
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl_args.semid = 42;
  mock.args.semctl_args.semnum = 0;
  mock.args.semctl_args.cmd = SETVAL;
  mock.args.semctl_args.arg.val = 1;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  SemaphoreV *sem = SemaphoreV::createExclusive(key, 0xFFFFFFFF, 1);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateExclusiveFailsWhenExists) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  try {
    SemaphoreV::createExclusive(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EEXIST);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateExclusiveFailsOnSemctlError) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl_args.semid = 42;
  mock.args.semctl_args.semnum = 0;
  mock.args.semctl_args.cmd = SETVAL;
  mock.args.semctl_args.arg.val = 1;
  mock.return_value = -1;
  mock.errno_value = ERANGE;
  mock_push_expected_call(mock);

  try {
    SemaphoreV::createExclusive(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ERANGE);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, OpenSucceeds) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0; // No creation flags for open
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // Set up the expected sembuf array
  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};

  mock.syscall = MOCK_SEMOP;
  mock.args.semop_args.semid = 42;
  mock.args.semop_args.nsops = 1;
  mock.args.semop_args.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  SemaphoreV *sem = SemaphoreV::open(key);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, OpenFailsWhenNotExists) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0;
  mock.return_value = -1;
  mock.errno_value = ENOENT;
  mock_push_expected_call(mock);

  try {
    SemaphoreV::open(key);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOENT);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, OpenFailsOnSemopError) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // Set up the expected sembuf array
  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};

  mock.syscall = MOCK_SEMOP;
  mock.args.semop_args.semid = 42;
  mock.args.semop_args.nsops = 1;
  mock.args.semop_args.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = ENOSPC;
  mock_push_expected_call(mock);

  try {
    SemaphoreV::open(key);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOSPC);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, OpenSucceedsAfterInterrupts) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // Set up the expected sembuf array - same for all three calls
  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};

  // First semop call - interrupted
  mock.syscall = MOCK_SEMOP;
  mock.args.semop_args.semid = 42;
  mock.args.semop_args.nsops = 1;
  mock.args.semop_args.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Second semop call - interrupted again
  mock.syscall = MOCK_SEMOP;
  mock.args.semop_args.semid = 42;
  mock.args.semop_args.nsops = 1;
  mock.args.semop_args.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Third semop call - succeeds
  mock.syscall = MOCK_SEMOP;
  mock.args.semop_args.semid = 42;
  mock.args.semop_args.nsops = 1;
  mock.args.semop_args.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  SemaphoreV *sem = SemaphoreV::open(key);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateSucceedsFirstTry) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl_args.semid = 42;
  mock.args.semctl_args.semnum = 0;
  mock.args.semctl_args.cmd = SETVAL;
  mock.args.semctl_args.arg.val = 1;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  SemaphoreV *sem = SemaphoreV::create(key, 0xFFFFFFFF, 1);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateSucceedsWithExisting) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  // First try fails with EEXIST
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  // Second try succeeds
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 0;
  mock.args.semget_args.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // Increment ref count
  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop_args.semid = 42;
  mock.args.semop_args.nsops = 1;
  mock.args.semop_args.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  SemaphoreV *sem = SemaphoreV::create(key, 0xFFFFFFFF, 1);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateSucceedsAfterRace) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  // First try fails with EEXIST
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  // Second try fails with ENOENT (race - someone deleted it)
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 0;
  mock.args.semget_args.semflg = 0;
  mock.return_value = -1;
  mock.errno_value = ENOENT;
  mock_push_expected_call(mock);

  // Third try succeeds (loop iteration)
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // Set initial value
  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl_args.semid = 42;
  mock.args.semctl_args.semnum = 0;
  mock.args.semctl_args.cmd = SETVAL;
  mock.args.semctl_args.arg.val = 1;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  SemaphoreV *sem = SemaphoreV::create(key, 0xFFFFFFFF, 1);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateSucceedsAfterSemopInterrupts) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  // First try fails with EEXIST
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  // Second try succeeds
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 0;
  mock.args.semget_args.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // Set up the expected sembuf array - same for all three calls
  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};

  // First semop call - interrupted
  mock.syscall = MOCK_SEMOP;
  mock.args.semop_args.semid = 42;
  mock.args.semop_args.nsops = 1;
  mock.args.semop_args.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Second semop call - interrupted again
  mock.syscall = MOCK_SEMOP;
  mock.args.semop_args.semid = 42;
  mock.args.semop_args.nsops = 1;
  mock.args.semop_args.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Third semop call - succeeds
  mock.syscall = MOCK_SEMOP;
  mock.args.semop_args.semid = 42;
  mock.args.semop_args.nsops = 1;
  mock.args.semop_args.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  SemaphoreV *sem = SemaphoreV::create(key, 0xFFFFFFFF, 1);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

// Error path tests
TEST_F(SemaphoreVTest, CreateFailsOnFirstSemgetError) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EACCES;
  mock_push_expected_call(mock);

  try {
    SemaphoreV::create(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EACCES);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateFailsOnSecondSemgetError) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  // First try fails with EEXIST
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  // Second try fails with EINVAL
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 0;
  mock.args.semget_args.semflg = 0;
  mock.return_value = -1;
  mock.errno_value = EINVAL;
  mock_push_expected_call(mock);

  try {
    SemaphoreV::create(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EINVAL);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateFailsOnSemctlError) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl_args.semid = 42;
  mock.args.semctl_args.semnum = 0;
  mock.args.semctl_args.cmd = SETVAL;
  mock.args.semctl_args.arg.val = 1;
  mock.return_value = -1;
  mock.errno_value = ERANGE;
  mock_push_expected_call(mock);

  try {
    SemaphoreV::create(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ERANGE);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateFailsOnSemopError) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  // First try fails with EEXIST
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  // Second try succeeds
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 0;
  mock.args.semget_args.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // semop fails
  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop_args.semid = 42;
  mock.args.semop_args.nsops = 1;
  mock.args.semop_args.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = ENOSPC;
  mock_push_expected_call(mock);

  try {
    SemaphoreV::create(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOSPC);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateFailsAfterSemopInterrupt) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  // First try fails with EEXIST
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  // Second try succeeds
  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 0;
  mock.args.semget_args.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};

  // First semop call - interrupted
  mock.syscall = MOCK_SEMOP;
  mock.args.semop_args.semid = 42;
  mock.args.semop_args.nsops = 1;
  mock.args.semop_args.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Second semop call - fails
  mock.syscall = MOCK_SEMOP;
  mock.args.semop_args.semid = 42;
  mock.args.semop_args.nsops = 1;
  mock.args.semop_args.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = ENOSPC;
  mock_push_expected_call(mock);

  try {
    SemaphoreV::create(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOSPC);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, UnlinkSucceeds) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl_args.semid = 42;
  mock.args.semctl_args.semnum = 0;
  mock.args.semctl_args.cmd = IPC_RMID;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  SemaphoreV::unlink(key);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, UnlinkFailsOnSemget) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0;
  mock.return_value = -1;
  mock.errno_value = ENOENT;
  mock_push_expected_call(mock);

  try {
    SemaphoreV::unlink(key);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOENT);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, UnlinkFailsOnSemctl) {
  MockCall mock{};
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = __FILE__;
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  Token key(__FILE__, 42);
  EXPECT_EQ(key.valueOf(), 1234);

  mock.syscall = MOCK_SEMGET;
  mock.args.semget_args.key = key.valueOf();
  mock.args.semget_args.nsems = 2;
  mock.args.semget_args.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl_args.semid = 42;
  mock.args.semctl_args.semnum = 0;
  mock.args.semctl_args.cmd = IPC_RMID;
  mock.return_value = -1;
  mock.errno_value = EPERM;
  mock_push_expected_call(mock);

  try {
    SemaphoreV::unlink(key);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EPERM);
  }

  mock_reset();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}