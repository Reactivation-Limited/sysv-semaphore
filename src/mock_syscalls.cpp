#include "mock_syscalls.hpp"
#include "errnoname.hpp"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>

static const char *const syscall_names[4] = {"MOCK_SEMGET", "MOCK_SEMOP", "MOCK_SEMCTL", "MOCK_FTOK"};

static std::queue<MockCall> call_queue;

void mock_push_expected_call(MockCall call) {
  printf("push a call to %s %d %d\n", syscall_names[call.syscall], call.return_value, call.errno_value);
  call_queue.push(call);
  printf("pushed a call, queue size: %zu\n", call_queue.size());
}

void mock_reset(void) {
  while (!call_queue.empty()) {
    call_queue.pop();
  }
}

static MockCall pop_call(MockSyscall expected_syscall) {
  printf("pop a mock call %s\n", syscall_names[expected_syscall]);

  if (call_queue.empty()) {
    fprintf(stderr, "[MOCK] No calls left in queue\n");
    exit(1);
  }

  MockCall call = call_queue.front();
  call_queue.pop();

  if (call.syscall != expected_syscall) {
    fprintf(stderr, "[MOCK] Expected syscall %s but got %s\n", syscall_names[expected_syscall],
            syscall_names[call.syscall]);
    exit(1);
  }
  errno = call.errno_value;
  return call;
}

// ---------------- Mocks ------------------

extern "C" int semget(key_t key, int nsems, int semflg) {
  printf("mock semget %d %d %o\n", key, nsems, semflg);
  MockCall call = pop_call(MOCK_SEMGET);
  if (call.args.semget_args.key != key || call.args.semget_args.nsems != nsems ||
      call.args.semget_args.semflg != semflg) {
    fprintf(stderr, "[MOCK] semget args mismatch\n");
    exit(1);
  }
  printf("mock semget returned %d errno %s\n", call.return_value, errnoname(errno));
  return call.return_value;
}

extern "C" int semop(int semid, struct sembuf *sops, size_t nsops) {
  printf("mock semop %x %p %zu\n", semid, sops, nsops);
  MockCall call = pop_call(MOCK_SEMOP);
  if (call.args.semop_args.semid != semid || call.args.semop_args.nsops != nsops) {
    fprintf(stderr, "[MOCK] semop args mismatch\n");
    exit(1);
  }
  return call.return_value;
}

extern "C" int semctl(int semid, int semnum, int cmd, ...) {
  printf("mock semctl %d %d %d\n", semid, semnum, cmd);
  MockCall call = pop_call(MOCK_SEMCTL);
  if (call.args.semctl_args.semid != semid || call.args.semctl_args.semnum != semnum ||
      call.args.semctl_args.cmd != cmd) {
    fprintf(stderr, "[MOCK] semctl args mismatch\n");
    exit(1);
  }
  return call.return_value;
}

extern "C" key_t ftok(const char *pathname, int proj_id) {
  printf("mock ftok %s %d\n", pathname, proj_id);
  MockCall call = pop_call(MOCK_FTOK);
  if (strcmp(call.args.ftok_args.pathname, pathname) != 0 || call.args.ftok_args.proj_id != proj_id) {
    fprintf(stderr, "[MOCK] ftok args mismatch\n");
    exit(1);
  }
  return call.return_value;
}