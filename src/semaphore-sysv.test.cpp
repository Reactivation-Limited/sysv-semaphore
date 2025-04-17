#include "semaphore-sysv.h"
#include "mock/syscalls.h"
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
    mock_push_expected_call({.syscall = MOCK_FTOK,
                             .return_value = 1234,
                             .errno_value = 0,
                             .args = {.ftok_args = {.pathname = __FILE__, .proj_id = 42}}});

    Token key(__FILE__, 42);
    EXPECT_EQ(key.valueOf(), 1234);
    return key;
  }

  SemaphoreV *createSemaphore() {
    Token key = createToken();

    mock_push_expected_call(
        {.syscall = MOCK_SEMGET,
         .return_value = 42,
         .errno_value = 0,
         .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

    mock_push_expected_call({.syscall = MOCK_SEMCTL,
                             .return_value = 0,
                             .errno_value = 0,
                             .args = {.semctl = {.semid = 42, .semnum = 0, .cmd = SETVAL, .arg = {.val = 1}}}});

    SemaphoreV *sem = SemaphoreV::createExclusive(key, 0xFFFFFFFF, 1);
    EXPECT_NE(sem, nullptr);
    return sem;
  }
};

TEST_F(SemaphoreVTest, CreateExclusiveSucceeds) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = 42,
       .errno_value = 0,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 42, .semnum = 0, .cmd = SETVAL, .arg = {.val = 1}}}});

  SemaphoreV *sem = SemaphoreV::createExclusive(key, 0xFFFFFFFF, 1);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateExclusiveFailsWhenExists) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = -1,
       .errno_value = EEXIST,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  try {
    SemaphoreV::createExclusive(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EEXIST);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateExclusiveFailsOnSemctl) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = 42,
       .errno_value = 0,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = -1,
                           .errno_value = ERANGE,
                           .args = {.semctl = {.semid = 42, .semnum = 0, .cmd = SETVAL, .arg = {.val = 1}}}});

  try {
    SemaphoreV::createExclusive(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ERANGE);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateExclusiveFailsOnSemget) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = -1,
       .errno_value = EACCES,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  try {
    SemaphoreV::createExclusive(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EACCES);
  }
}

TEST_F(SemaphoreVTest, OpenSucceeds) {
  Token key = createToken();

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 42,
                           .errno_value = 0,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0}}});

  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  SemaphoreV *sem = SemaphoreV::open(key);
  EXPECT_NE(sem, nullptr);
  delete sem;
}

TEST_F(SemaphoreVTest, OpenFailsWhenNotExists) {
  Token key = createToken();

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = -1,
                           .errno_value = ENOENT,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0}}});

  try {
    SemaphoreV::open(key);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOENT);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, OpenFailsOnSemopError) {
  Token key = createToken();

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 42,
                           .errno_value = 0,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0}}});

  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = ENOSPC,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  try {
    SemaphoreV::open(key);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOSPC);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, OpenSucceedsAfterInterrupts) {
  Token key = createToken();

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 42,
                           .errno_value = 0,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0}}});

  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EINTR,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EINTR,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  SemaphoreV *sem = SemaphoreV::open(key);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateSucceedsFirstTry) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = 42,
       .errno_value = 0,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 42, .semnum = 0, .cmd = SETVAL, .arg = {.val = 1}}}});

  SemaphoreV *sem = SemaphoreV::create(key, 0xFFFFFFFF, 1);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateSucceedsWithExisting) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = -1,
       .errno_value = EEXIST,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 42,
                           .errno_value = 0,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 0, .semflg = 0}}});

  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  SemaphoreV *sem = SemaphoreV::create(key, 0xFFFFFFFF, 1);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateSucceedsAfterRace) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = -1,
       .errno_value = EEXIST,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = -1,
                           .errno_value = ENOENT,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 0, .semflg = 0}}});

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = 42,
       .errno_value = 0,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 42, .semnum = 0, .cmd = SETVAL, .arg = {.val = 1}}}});

  SemaphoreV *sem = SemaphoreV::create(key, 0xFFFFFFFF, 1);
  EXPECT_NE(sem, nullptr);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateSucceedsAfterSemopInterrupts) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = -1,
       .errno_value = EEXIST,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 42,
                           .errno_value = 0,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 0, .semflg = 0}}});

  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EINTR,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = ENOSPC,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  try {
    SemaphoreV::create(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOSPC);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateFailsOnFirstSemgetError) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = -1,
       .errno_value = EACCES,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  try {
    SemaphoreV::create(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EACCES);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateFailsOnSecondSemgetError) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = -1,
       .errno_value = EEXIST,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = -1,
                           .errno_value = EINVAL,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 0, .semflg = 0}}});

  try {
    SemaphoreV::create(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EINVAL);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateFailsOnSemctlError) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = 42,
       .errno_value = 0,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = -1,
                           .errno_value = ERANGE,
                           .args = {.semctl = {.semid = 42, .semnum = 0, .cmd = SETVAL, .arg = {.val = 1}}}});

  try {
    SemaphoreV::create(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ERANGE);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateFailsOnSemopError) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = -1,
       .errno_value = EEXIST,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 42,
                           .errno_value = 0,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 0, .semflg = 0}}});

  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = ENOSPC,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  try {
    SemaphoreV::create(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOSPC);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, CreateFailsAfterSemopInterrupt) {
  Token key = createToken();

  mock_push_expected_call(
      {.syscall = MOCK_SEMGET,
       .return_value = -1,
       .errno_value = EEXIST,
       .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0777 | IPC_CREAT | IPC_EXCL}}});

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 42,
                           .errno_value = 0,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 0, .semflg = 0}}});

  struct sembuf expected_sops[1] = {{1, 1, SEM_UNDO}};

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EINTR,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = ENOSPC,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  try {
    SemaphoreV::create(key, 0xFFFFFFFF, 1);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOSPC);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, UnlinkSucceeds) {
  Token key = createToken();

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 42,
                           .errno_value = 0,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0}}});

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 42, .semnum = 0, .cmd = IPC_RMID}}});

  SemaphoreV::unlink(key);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, UnlinkFailsOnSemget) {
  Token key = createToken();

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = -1,
                           .errno_value = ENOENT,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0}}});

  try {
    SemaphoreV::unlink(key);
    FAIL() << "Expected std::system_error";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOENT);
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, UnlinkFailsOnSemctl) {
  Token key = createToken();

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 42,
                           .errno_value = 0,
                           .args = {.semget = {.key = key.valueOf(), .nsems = 2, .semflg = 0}}});

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = -1,
                           .errno_value = EPERM,
                           .args = {.semctl = {.semid = 42, .semnum = 0, .cmd = IPC_RMID}}});

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

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 5,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 42, .semnum = 0, .cmd = GETVAL}}});

  EXPECT_EQ(sem->valueOf(), 5);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, ValueOfFails) {
  SemaphoreV *sem = createSemaphore();

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = -1,
                           .errno_value = EPERM,
                           .args = {.semctl = {.semid = 42, .semnum = 0, .cmd = GETVAL}}});

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

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 3,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 42, .semnum = 1, .cmd = GETVAL}}});

  EXPECT_EQ(sem->refs(), 3);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, RefsFails) {
  SemaphoreV *sem = createSemaphore();

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = -1,
                           .errno_value = EPERM,
                           .args = {.semctl = {.semid = 42, .semnum = 1, .cmd = GETVAL}}});

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

  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  sem->wait();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, WaitValueSucceeds) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{0, -3, SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  sem->wait(3);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, WaitSucceedsAfterInterrupt) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO}};

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EINTR,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EINTR,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  sem->wait();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, WaitFails) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EAGAIN,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  try {
    sem->wait();
    FAIL() << "Expected std::system_error with EAGAIN";
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), EAGAIN);
    EXPECT_EQ(e.code().category(), std::system_category());
  }

  mock_reset();
}

TEST_F(SemaphoreVTest, TryWaitSucceeds) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO | IPC_NOWAIT}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  EXPECT_TRUE(sem->trywait());
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, TryWaitValueSucceeds) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{0, -3, SEM_UNDO | IPC_NOWAIT}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  EXPECT_TRUE(sem->trywait(3));
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, TryWaitWouldBlock) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO | IPC_NOWAIT}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EAGAIN,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  EXPECT_FALSE(sem->trywait());
  EXPECT_EQ(errno, EAGAIN);

  mock_reset();
}

TEST_F(SemaphoreVTest, TryWaitSucceedsAfterInterrupts) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO | IPC_NOWAIT}};

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EINTR,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EINTR,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  EXPECT_TRUE(sem->trywait());
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, TryWaitFails) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{0, -1, SEM_UNDO | IPC_NOWAIT}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = ERANGE,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

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

  struct sembuf expected_sops[1] = {{0, 1, SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  sem->post();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, PostValueSucceeds) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{0, 3, SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  sem->post(3);
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, PostSucceedsAfterInterrupts) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{0, 1, SEM_UNDO}};

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EINTR,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EINTR,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  sem->post();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CloseSucceeds) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{1, -1, IPC_NOWAIT | SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  sem->close();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CloseWithEagainAndRmidSucceeds) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{1, -1, IPC_NOWAIT | SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EAGAIN,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 42, .semnum = 0, .cmd = IPC_RMID}}});

  sem->close();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CloseWithEagainAndRmidFails) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{1, -1, IPC_NOWAIT | SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EAGAIN,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = -1,
                           .errno_value = EPERM,
                           .args = {.semctl = {.semid = 42, .semnum = 0, .cmd = IPC_RMID}}});

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

  struct sembuf expected_sops[1] = {{1, -1, IPC_NOWAIT | SEM_UNDO}};

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EINTR,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = EINTR,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

  sem->close();
  EXPECT_EQ(errno, 0);

  mock_reset();
}

TEST_F(SemaphoreVTest, CloseFails) {
  SemaphoreV *sem = createSemaphore();

  struct sembuf expected_sops[1] = {{1, -1, IPC_NOWAIT | SEM_UNDO}};
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = -1,
                           .errno_value = ENOSPC,
                           .args = {.semop = {.semid = 42, .sops = expected_sops, .nsops = 1}}});

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