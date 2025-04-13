#include "semaphore-sysv.h"
#include <cerrno>
#include <errnoname.c>
#include <errnoname.h>
#include <gtest/gtest.h>
#include <sys/sem.h>

// Mock functions for syscalls
extern "C" {
int mock_semget(key_t key, int nsems, int semflg) {
  errno = EACCES; // Simulate permission denied
  printf("%d", errno);
  return -1;
}
}

// Replace the real syscalls with our mocks
// haha does not work, obviously as not defined in the subject code

#define semget mock_semget

class SemaphoreVTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Reset errno before each test
    errno = 0;
  }
};

TEST_F(SemaphoreVTest, OpenFailsWhenSemgetFails) {
  Token key(__FILE__, 42);

  // Test that open throws when semget fails
  EXPECT_THROW({ SemaphoreV::open(key); }, std::system_error);

  // Verify the error code and message
  try {
    SemaphoreV::open(key);
  } catch (const std::system_error &e) {
    EXPECT_EQ(e.code().value(), ENOENT);
    EXPECT_STREQ(e.what(), "semget: No such file or directory");
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}