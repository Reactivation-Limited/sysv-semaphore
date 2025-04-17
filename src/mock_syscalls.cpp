#include "mock_syscalls.hpp"
#include "errnoname.hpp"
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>

static const char *const syscall_names[4] = {"MOCK_SEMGET", "MOCK_SEMOP", "MOCK_SEMCTL", "MOCK_FTOK"};

static std::queue<MockCall> call_queue;

void mock_push_expected_call(MockCall call) { call_queue.push(call); }

void mock_reset(void) {
  while (!call_queue.empty()) {
    call_queue.pop();
  }
}

static MockCall *pop_call(MockSyscall expected_syscall, bool *args_match) {
  *args_match = true;
  static MockCall call;

  if (call_queue.empty()) {
    fprintf(stderr, "[MOCK] No call queued\n");
    errno = ENOSYS; // Function not implemented
    return nullptr;
  }

  call = call_queue.front();
  call_queue.pop();

  if (call.syscall != expected_syscall) {
    fprintf(stderr, "[MOCK] Expected syscall %s but got %s\n", syscall_names[expected_syscall],
            syscall_names[call.syscall]);
    errno = ENODATA; // No data available (args mismatch)
    *args_match = false;
    return nullptr;
  }

  errno = call.errno_value;
  return &call;
}

// ---------------- Mocks ------------------

extern "C" int semget(key_t key, int nsems, int semflg) {
  bool args_match;
  MockCall *call = pop_call(MOCK_SEMGET, &args_match);
  if (!call)
    return -1;

  if (call->args.semget.key != key || call->args.semget.nsems != nsems || call->args.semget.semflg != semflg) {
    fprintf(
        stderr,
        "[MOCK] semget args mismatch: called with key=%d nsems=%d semflg=%d but expected key=%d nsems=%d semflg=%d\n",
        key, nsems, semflg, call->args.semget.key, call->args.semget.nsems, call->args.semget.semflg);
    errno = ENODATA; // No data available (args mismatch)
    return -1;
  }
  return call->return_value;
}

extern "C" int semop(int semid, struct sembuf *sops, size_t nsops) {
  bool args_match;
  MockCall *call = pop_call(MOCK_SEMOP, &args_match);
  if (!call)
    return -1;

  if (call->args.semop.semid != semid || call->args.semop.nsops != nsops) {
    fprintf(stderr, "[MOCK] semop args mismatch: called with semid=%d nsops=%zu but expected semid=%d nsops=%zu\n",
            semid, nsops, call->args.semop.semid, call->args.semop.nsops);
    errno = ENODATA;
    return -1;
  }

  // Validate each sembuf in the array
  for (size_t i = 0; i < nsops; i++) {
    const sembuf &op = call->args.semop.sops[i];
    if (op.sem_num != sops[i].sem_num || op.sem_op != sops[i].sem_op || op.sem_flg != sops[i].sem_flg) {
      fprintf(stderr,
              "[MOCK] semop args mismatch: called with sembuf[%zu]={sem_num=%d, sem_op=%d, sem_flg=%d} but expected "
              "sembuf[%zu]={sem_num=%d, sem_op=%d, sem_flg=%d}\n",
              i, sops[i].sem_num, sops[i].sem_op, sops[i].sem_flg, i, op.sem_num, op.sem_op, op.sem_flg);
      errno = ENODATA;
      return -1;
    }
  }

  return call->return_value;
}

extern "C" int semctl(int semid, int semnum, int cmd, ...) {
  va_list ap;
  va_start(ap, cmd);
  semun arg = va_arg(ap, semun);
  va_end(ap);

  bool args_match;
  MockCall *call = pop_call(MOCK_SEMCTL, &args_match);
  if (!call)
    return -1;

  if (call->args.semctl.semid != semid || call->args.semctl.semnum != semnum || call->args.semctl.cmd != cmd) {
    fprintf(
        stderr,
        "[MOCK] semctl args mismatch: called with semid=%d semnum=%d cmd=%d but expected semid=%d semnum=%d cmd=%d\n",
        semid, semnum, cmd, call->args.semctl.semid, call->args.semctl.semnum, call->args.semctl.cmd);
    errno = ENODATA;
    return -1;
  }

  if (cmd == SETVAL) {
    if (arg.val != call->args.semctl.arg.val) {
      fprintf(stderr, "[MOCK] semctl args mismatch: called with arg.val=%d but expected arg.val=%d\n", arg.val,
              call->args.semctl.arg.val);
      errno = ENODATA;
      return -1;
    }
  }

  return call->return_value;
}

extern "C" key_t ftok(const char *pathname, int proj_id) {
  bool args_match;
  MockCall *call = pop_call(MOCK_FTOK, &args_match);
  if (!call)
    return -1;

  if (strcmp(call->args.ftok_args.pathname, pathname) != 0 || call->args.ftok_args.proj_id != proj_id) {
    fprintf(stderr,
            "[MOCK] ftok args mismatch: called with pathname=%s proj_id=%d but expected pathname=%s proj_id=%d\n",
            pathname, proj_id, call->args.ftok_args.pathname, call->args.ftok_args.proj_id);
    errno = ENODATA;
    return -1;
  }

  return call->return_value;
}