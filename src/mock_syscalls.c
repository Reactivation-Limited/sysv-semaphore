#define _GNU_SOURCE
#include "mock_syscalls.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#define MAX_MOCK_CALLS 128

static MockCall call_stack[MAX_MOCK_CALLS];
static int stack_top = -1;

void mock_push_expected_call(MockCall call) {
  if (stack_top >= MAX_MOCK_CALLS - 1) {
    fprintf(stderr, "[MOCK] Stack overflow\n");
    exit(1);
  }
  printf("pushed a call\n");
  call_stack[++stack_top] = call;
}

void mock_reset(void) { stack_top = -1; }

static MockCall pop_call(MockSyscall expected_syscall) {
  if (stack_top < 0) {
    fprintf(stderr, "[MOCK] No calls left on stack\n");
    exit(1);
  }

  MockCall call = call_stack[stack_top--];
  if (call.syscall != expected_syscall) {
    fprintf(stderr, "[MOCK] Expected syscall %d but got %d\n", call.syscall, expected_syscall);
    exit(1);
  }
  errno = call.errno_value;
  return call;
}

// ---------------- Mocks ------------------

int semget(key_t key, int nsems, int semflg) {
  MockCall call = pop_call(MOCK_SEMGET);
  if (call.args.semget_args.key != key || call.args.semget_args.nsems != nsems ||
      call.args.semget_args.semflg != semflg) {
    fprintf(stderr, "[MOCK] semget args mismatch\n");
    exit(1);
  }
  return call.return_value;
}

int semop(int semid, struct sembuf *sops, size_t nsops) {
  MockCall call = pop_call(MOCK_SEMOP);
  if (call.args.semop_args.semid != semid || call.args.semop_args.nsops != nsops) {
    fprintf(stderr, "[MOCK] semop args mismatch\n");
    exit(1);
  }
  return call.return_value;
}

int semctl(int semid, int semnum, int cmd, ...) {
  MockCall call = pop_call(MOCK_SEMCTL);
  if (call.args.semctl_args.semid != semid || call.args.semctl_args.semnum != semnum ||
      call.args.semctl_args.cmd != cmd) {
    fprintf(stderr, "[MOCK] semctl args mismatch\n");
    exit(1);
  }
  return call.return_value;
}

key_t ftok(const char *pathname, int proj_id) {
  MockCall call = pop_call(MOCK_FTOK);
  if (strcmp(call.args.ftok_args.pathname, pathname) != 0 || call.args.ftok_args.proj_id != proj_id) {
    fprintf(stderr, "[MOCK] ftok args mismatch\n");
    exit(1);
  }
  return call.return_value;
}
