#pragma once

#include <queue>
#include <stdexcept>
#include <string>
#include <sys/ipc.h>
#include <sys/sem.h>

#ifdef __cplusplus
extern "C" {
#endif

class MockFailure : public std::runtime_error {
public:
  explicit MockFailure(const std::string &message) : std::runtime_error(message) {}
};

typedef enum { MOCK_SEMGET, MOCK_SEMOP, MOCK_SEMCTL, MOCK_FTOK } MockSyscall;

typedef struct {
  MockSyscall syscall;
  union {
    struct {
      key_t key;
      int nsems;
      int semflg;
    } semget_args;

    struct {
      int semid;
      struct sembuf *sops;
      size_t nsops;
    } semop_args;

    struct {
      int semid;
      int semnum;
      int cmd;
    } semctl_args;

    struct {
      const char *pathname;
      int proj_id;
    } ftok_args;
  } args;

  int return_value;
  int errno_value;

} MockCall;

void mock_push_expected_call(MockCall call);
void mock_reset(void);

#ifdef __cplusplus
}
#endif