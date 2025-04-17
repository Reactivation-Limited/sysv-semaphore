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

  Token createToken() {
    MockCall mock{};
    mock.syscall = MOCK_FTOK;
    mock.args.ftok_args.pathname = __FILE__;
    mock.args.ftok_args.proj_id = 42;
    mock.return_value = 1234;
    mock.errno_value = 0;
    mock_push_expected_call(mock);

    Token key(__FILE__, 42);
    EXPECT_EQ(key.valueOf(), 1234);
    return key;
  }

  SemaphoreV *createSemaphore() {
    Token key = createToken();

    MockCall mock{};
    mock.syscall = MOCK_SEMGET;
    mock.args.semget.key = key.valueOf();
    mock.args.semget.nsems = 2;
    mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
    mock.return_value = 42;
    mock.errno_value = 0;
    mock_push_expected_call(mock);

    mock.syscall = MOCK_SEMCTL;
    mock.args.semctl.semid = 42;
    mock.args.semctl.semnum = 0;
    mock.args.semctl.cmd = SETVAL;
    mock.args.semctl.arg.val = 1;
    mock.return_value = 0;
    mock.errno_value = 0;
    mock_push_expected_call(mock);

    SemaphoreV *sem = SemaphoreV::createExclusive(key, 0xFFFFFFFF, 1);
    EXPECT_NE(sem, nullptr);
    return sem;
  }
};

TEST_F(SemaphoreVTest, CreateExclusiveSucceeds) {
  MockCall mock{};
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL; // check the mode is masked
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 0;
  mock.args.semctl.cmd = SETVAL;
  mock.args.semctl.arg.val = 1;
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
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
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
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 0;
  mock.args.semctl.cmd = SETVAL;
  mock.args.semctl.arg.val = 1;
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
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0; // No creation flags for open
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // Set up the expected sembuf array
  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};

  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
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
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0;
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
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // Set up the expected sembuf array
  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};

  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
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
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // Set up the expected sembuf array - same for all three calls
  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};

  // First semop call - interrupted
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Second semop call - interrupted again
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Third semop call - succeeds
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
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
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 0;
  mock.args.semctl.cmd = SETVAL;
  mock.args.semctl.arg.val = 1;
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
  Token key = createToken();

  // First try fails with EEXIST
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  // Second try succeeds
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 0;
  mock.args.semget.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // Increment ref count
  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
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
  Token key = createToken();

  // First try fails with EEXIST
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  // Second try fails with ENOENT (race - someone deleted it)
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 0;
  mock.args.semget.semflg = 0;
  mock.return_value = -1;
  mock.errno_value = ENOENT;
  mock_push_expected_call(mock);

  // Third try succeeds (loop iteration)
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // Set initial value
  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 0;
  mock.args.semctl.cmd = SETVAL;
  mock.args.semctl.arg.val = 1;
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
  Token key = createToken();

  // First try fails with EEXIST
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  // Second try succeeds
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 0;
  mock.args.semget.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // Set up the expected sembuf array - same for all three calls
  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};

  // First semop call - interrupted
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Second semop call - interrupted again
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Third semop call - succeeds
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
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
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
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
  Token key = createToken();

  // First try fails with EEXIST
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  // Second try fails with EINVAL
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 0;
  mock.args.semget.semflg = 0;
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
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 0;
  mock.args.semctl.cmd = SETVAL;
  mock.args.semctl.arg.val = 1;
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
  Token key = createToken();

  // First try fails with EEXIST
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  // Second try succeeds
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 0;
  mock.args.semget.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  // semop fails
  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
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
  Token key = createToken();

  // First try fails with EEXIST
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0777 | IPC_CREAT | IPC_EXCL;
  mock.return_value = -1;
  mock.errno_value = EEXIST;
  mock_push_expected_call(mock);

  // Second try succeeds
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 0;
  mock.args.semget.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};

  // First semop call - interrupted
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Second semop call - fails
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
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
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 0;
  mock.args.semctl.cmd = IPC_RMID;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  SemaphoreV::unlink(key);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, UnlinkFailsOnSemget) {
  MockCall mock{};
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0;
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
  Token key = createToken();

  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = key.valueOf();
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0;
  mock.return_value = 42;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 0;
  mock.args.semctl.cmd = IPC_RMID;
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

TEST_F(SemaphoreVTest, ValueOfSucceeds) {
  SemaphoreV *sem = createSemaphore();

  // Test valueOf
  MockCall mock{};
  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 0;
  mock.args.semctl.cmd = GETVAL;
  mock.return_value = 5; // Current value of the semaphore
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  EXPECT_EQ(sem->valueOf(), 5);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, ValueOfFails) {
  SemaphoreV *sem = createSemaphore();

  // Test valueOf failure
  MockCall mock{};
  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 0;
  mock.args.semctl.cmd = GETVAL;
  mock.return_value = -1;
  mock.errno_value = EPERM;
  mock_push_expected_call(mock);

  try {
    sem->valueOf();
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EPERM);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, RefsSucceeds) {
  SemaphoreV *sem = createSemaphore();

  // Test refs
  MockCall mock{};
  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 1; // REF_COUNT
  mock.args.semctl.cmd = GETVAL;
  mock.return_value = 3; // Current reference count
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  EXPECT_EQ(sem->refs(), 3);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, RefsFails) {
  SemaphoreV *sem = createSemaphore();

  // Test refs failure
  MockCall mock{};
  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 1; // REF_COUNT
  mock.args.semctl.cmd = GETVAL;
  mock.return_value = -1;
  mock.errno_value = EPERM;
  mock_push_expected_call(mock);

  try {
    sem->refs();
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EPERM);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, WaitSucceeds) {
  SemaphoreV *sem = createSemaphore();

  // Test wait()
  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  sem->wait();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, WaitValueSucceeds) {
  SemaphoreV *sem = createSemaphore();

  // Test wait(3)
  struct sembuf expected_sops[1] = {{0, -3, SEM_UNDO}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  sem->wait(3);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, WaitSucceedsAfterInterrupt) {
  SemaphoreV *sem = createSemaphore();

  // Test wait with EINTR
  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO}};

  // First attempt - interrupted
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Second attempt - interrupted
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Third attempt - succeeds
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  sem->wait();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, WaitFails) {
  SemaphoreV *sem = createSemaphore();

  // Test wait failure
  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EAGAIN;
  mock_push_expected_call(mock);

  try {
    sem->wait();
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EAGAIN);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, TryWaitSucceeds) {
  SemaphoreV *sem = createSemaphore();

  // Test trywait success
  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO | IPC_NOWAIT}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  EXPECT_TRUE(sem->trywait());
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, TryWaitValueSucceeds) {
  SemaphoreV *sem = createSemaphore();

  // Test trywait with specific value
  struct sembuf expected_sops[1] = {{0, -3, SEM_UNDO | IPC_NOWAIT}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  EXPECT_TRUE(sem->trywait(3));
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, TryWaitWouldBlock) {
  SemaphoreV *sem = createSemaphore();

  // Test trywait returns false when EAGAIN
  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO | IPC_NOWAIT}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EAGAIN;
  mock_push_expected_call(mock);

  EXPECT_FALSE(sem->trywait());
  EXPECT_EQ(errno, EAGAIN);

  mock_reset();
}

TEST_F(SemaphoreVTest, TryWaitSucceedsAfterInterrupts) {
  SemaphoreV *sem = createSemaphore();

  // Test trywait with EINTR interrupts
  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO | IPC_NOWAIT}};

  // First attempt - interrupted
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Second attempt - interrupted
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Third attempt - succeeds
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  EXPECT_TRUE(sem->trywait());
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, TryWaitFails) {
  SemaphoreV *sem = createSemaphore();

  // Test trywait failure with ERANGE
  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO | IPC_NOWAIT}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = ERANGE;
  mock_push_expected_call(mock);

  try {
    sem->trywait();
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ERANGE);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, PostSucceeds) {
  SemaphoreV *sem = createSemaphore();

  // Test post success
  struct sembuf expected_sops[1] = {{0, 1, SEM_UNDO}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  sem->post();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, PostValueSucceeds) {
  SemaphoreV *sem = createSemaphore();

  // Test post with specific value
  struct sembuf expected_sops[1] = {{0, 3, SEM_UNDO}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  sem->post(3);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, PostSucceedsAfterInterrupts) {
  SemaphoreV *sem = createSemaphore();

  // Test post with EINTR interrupts
  struct sembuf expected_sops[1] = {{0, 1, SEM_UNDO}};

  // First attempt - interrupted
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Second attempt - interrupted
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Third attempt - succeeds
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  sem->post();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, PostFails) {
  SemaphoreV *sem = createSemaphore();

  // Test post failure with ENOSPC
  struct sembuf expected_sops[1] = {{0, 1, SEM_UNDO}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = ENOSPC;
  mock_push_expected_call(mock);

  try {
    sem->post();
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOSPC);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CloseSucceeds) {
  SemaphoreV *sem = createSemaphore();

  // Test close success
  struct sembuf expected_sops[1] = {{1, -1, IPC_NOWAIT | SEM_UNDO}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  sem->close();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CloseWithEagainAndRmidSucceeds) {
  SemaphoreV *sem = createSemaphore();

  // Test close with EAGAIN followed by successful IPC_RMID
  struct sembuf expected_sops[1] = {{1, -1, IPC_NOWAIT | SEM_UNDO}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EAGAIN;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 0;
  mock.args.semctl.cmd = IPC_RMID;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  sem->close();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CloseWithEagainAndRmidFails) {
  SemaphoreV *sem = createSemaphore();

  // Test close with EAGAIN followed by failed IPC_RMID
  struct sembuf expected_sops[1] = {{1, -1, IPC_NOWAIT | SEM_UNDO}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EAGAIN;
  mock_push_expected_call(mock);

  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 42;
  mock.args.semctl.semnum = 0;
  mock.args.semctl.cmd = IPC_RMID;
  mock.return_value = -1;
  mock.errno_value = EPERM;
  mock_push_expected_call(mock);

  try {
    sem->close();
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EPERM);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CloseSucceedsAfterInterrupts) {
  SemaphoreV *sem = createSemaphore();

  // Test close with EINTR interrupts
  struct sembuf expected_sops[1] = {{1, -1, IPC_NOWAIT | SEM_UNDO}};

  // First attempt - interrupted
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Second attempt - interrupted
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = EINTR;
  mock_push_expected_call(mock);

  // Third attempt - succeeds
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = 0;
  mock.errno_value = 0;
  mock_push_expected_call(mock);

  sem->close();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CloseFails) {
  SemaphoreV *sem = createSemaphore();

  // Test close failure with ENOSPC
  struct sembuf expected_sops[1] = {{1, -1, IPC_NOWAIT | SEM_UNDO}};
  MockCall mock{};
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 42;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = expected_sops;
  mock.return_value = -1;
  mock.errno_value = ENOSPC;
  mock_push_expected_call(mock);

  try {
    sem->close();
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOSPC);
  }

  mock_reset();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}