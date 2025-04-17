#include "mock_syscalls.hpp"
#include <gtest/gtest.h>
#include <sys/sem.h>

class MockSyscallsTest : public ::testing::Test {
protected:
  void SetUp() override { mock_reset(); }

  void TearDown() override { mock_reset(); }
};

TEST_F(MockSyscallsTest, EmptyQueueThrowsException) { EXPECT_THROW(ftok("test", 42), MockFailure); }

TEST_F(MockSyscallsTest, FtokMockWorks) {
  mock_push_expected_call({.syscall = MOCK_FTOK,
                           .return_value = 1234,
                           .errno_value = 0,
                           .args = {.ftok_args = {.pathname = "test", .proj_id = 42}}});

  EXPECT_EQ(ftok("test", 42), 1234);
}

TEST_F(MockSyscallsTest, SemgetMockWorks) {
  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 5678,
                           .errno_value = 0,
                           .args = {.semget = {.key = 1234, .nsems = 2, .semflg = 0600}}});

  EXPECT_EQ(semget(1234, 2, 0600), 5678);
}

TEST_F(MockSyscallsTest, SemopMockWorks) {
  struct sembuf ops[1] = {{0, 1, SEM_UNDO}};

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 1234, .sops = ops, .nsops = 1}}});

  EXPECT_EQ(semop(1234, ops, 1), 0);
}

TEST_F(MockSyscallsTest, SemctlMockWorks) {
  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 5,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 1234, .semnum = 0, .cmd = GETVAL}}});

  EXPECT_EQ(semctl(1234, 0, GETVAL), 5);
}

TEST_F(MockSyscallsTest, MockCallsRunInOrder) {
  // Push multiple calls in a specific order
  mock_push_expected_call({.syscall = MOCK_FTOK,
                           .return_value = 1234,
                           .errno_value = 0,
                           .args = {.ftok_args = {.pathname = "test", .proj_id = 42}}});

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 5678,
                           .errno_value = 0,
                           .args = {.semget = {.key = 1234, .nsems = 2, .semflg = 0600}}});

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 5678, .semnum = 0, .cmd = SETVAL, .arg = {.val = 1}}}});

  // Execute calls in the same order
  EXPECT_EQ(ftok("test", 42), 1234);
  EXPECT_EQ(semget(1234, 2, 0600), 5678);
  EXPECT_EQ(semctl(5678, 0, SETVAL, 1), 0);

  // Queue should be empty now
  EXPECT_THROW(ftok("test", 42), MockFailure);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}