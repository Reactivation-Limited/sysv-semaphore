#include "mock_syscalls.hpp"
#include <gtest/gtest.h>
#include <sys/sem.h>

class MockSyscallsTest : public ::testing::Test {
protected:
  void SetUp() override { mock_reset(); }

  void TearDown() override { mock_reset(); }
};

TEST_F(MockSyscallsTest, EmptyQueueThrowsException) {
  MockCall mock;
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = "test";
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;

  // No calls pushed, should throw
  EXPECT_THROW(ftok("test", 42), MockFailure);
}

TEST_F(MockSyscallsTest, FtokMockWorks) {
  MockCall mock;
  mock.syscall = MOCK_FTOK;
  mock.args.ftok_args.pathname = "test";
  mock.args.ftok_args.proj_id = 42;
  mock.return_value = 1234;
  mock.errno_value = 0;

  mock_push_expected_call(mock);
  EXPECT_EQ(ftok("test", 42), 1234);
}

TEST_F(MockSyscallsTest, SemgetMockWorks) {
  MockCall mock;
  mock.syscall = MOCK_SEMGET;
  mock.args.semget.key = 1234;
  mock.args.semget.nsems = 2;
  mock.args.semget.semflg = 0600;
  mock.return_value = 5678;
  mock.errno_value = 0;

  mock_push_expected_call(mock);
  EXPECT_EQ(semget(1234, 2, 0600), 5678);
}

TEST_F(MockSyscallsTest, SemopMockWorks) {
  struct sembuf ops[1] = {{0, 1, SEM_UNDO}};

  MockCall mock;
  mock.syscall = MOCK_SEMOP;
  mock.args.semop.semid = 1234;
  mock.args.semop.nsops = 1;
  mock.args.semop.sops = ops;
  mock.return_value = 0;
  mock.errno_value = 0;

  mock_push_expected_call(mock);
  EXPECT_EQ(semop(1234, ops, 1), 0);
}

TEST_F(MockSyscallsTest, SemctlMockWorks) {
  MockCall mock;
  mock.syscall = MOCK_SEMCTL;
  mock.args.semctl.semid = 1234;
  mock.args.semctl.semnum = 0;
  mock.args.semctl.cmd = GETVAL;
  mock.return_value = 5;
  mock.errno_value = 0;

  mock_push_expected_call(mock);
  EXPECT_EQ(semctl(1234, 0, GETVAL), 5);
}

TEST_F(MockSyscallsTest, MockCallsRunInOrder) {
  // Push multiple calls in a specific order
  MockCall mock1;
  mock1.syscall = MOCK_FTOK;
  mock1.args.ftok_args.pathname = "test";
  mock1.args.ftok_args.proj_id = 42;
  mock1.return_value = 1234;
  mock1.errno_value = 0;
  mock_push_expected_call(mock1);

  MockCall mock2;
  mock2.syscall = MOCK_SEMGET;
  mock2.args.semget.key = 1234;
  mock2.args.semget.nsems = 2;
  mock2.args.semget.semflg = 0600;
  mock2.return_value = 5678;
  mock2.errno_value = 0;
  mock_push_expected_call(mock2);

  MockCall mock3;
  mock3.syscall = MOCK_SEMCTL;
  mock3.args.semctl.semid = 5678;
  mock3.args.semctl.semnum = 0;
  mock3.args.semctl.cmd = SETVAL;
  mock3.args.semctl.arg.val = 1;
  mock3.return_value = 0;
  mock3.errno_value = 0;
  mock_push_expected_call(mock3);

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