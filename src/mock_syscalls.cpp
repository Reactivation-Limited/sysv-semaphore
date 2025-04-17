#include "mock_syscalls.hpp"
#include "errnoname.hpp"
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <sstream>

static const char *const syscall_names[4] = {"MOCK_SEMGET", "MOCK_SEMOP", "MOCK_SEMCTL", "MOCK_FTOK"};

static std::queue<MockCall> call_queue;

void mock_push_expected_call(MockCall call) { call_queue.push(call); }

void mock_reset(void) {
  while (!call_queue.empty()) {
    call_queue.pop();
  }
}

static MockCall pop_call(MockSyscall expected_syscall) {
  if (call_queue.empty()) {
    throw MockFailure("[MOCK] No calls left in queue");
  }

  MockCall call = call_queue.front();
  call_queue.pop();

  if (call.syscall != expected_syscall) {
    std::stringstream ss;
    ss << "[MOCK] Expected syscall " << syscall_names[expected_syscall] << " but got " << syscall_names[call.syscall];
    throw MockFailure(ss.str());
  }

  errno = call.errno_value;
  return call;
}

// ---------------- Mocks ------------------

extern "C" int semget(key_t key, int nsems, int semflg) {
  MockCall call = pop_call(MOCK_SEMGET);

  if (call.args.semget_args.key != key || call.args.semget_args.nsems != nsems ||
      call.args.semget_args.semflg != semflg) {
    std::stringstream ss;
    ss << "[MOCK] semget args mismatch: called with key=" << key << " nsems=" << nsems << std::oct
       << " semflg=" << std::oct << semflg << " but expected key=" << std::dec << call.args.semget_args.key
       << " nsems=" << call.args.semget_args.nsems << std::oct << " semflg=" << std::oct
       << call.args.semget_args.semflg;
    throw MockFailure(ss.str());
  }
  return call.return_value;
}

extern "C" int semop(int semid, struct sembuf *sops, size_t nsops) {
  MockCall call = pop_call(MOCK_SEMOP);

  // here need to check the sembuf, which gets complicated
  if (call.args.semop_args.semid != semid || call.args.semop_args.nsops != nsops) {
    std::stringstream ss;
    ss << "[MOCK] semop args mismatch: called with semid=" << semid << " nsops=" << nsops
       << " but expected semid=" << call.args.semop_args.semid << " nsops=" << call.args.semop_args.nsops;
    throw MockFailure(ss.str());
  }

  // Validate each sembuf in the array
  for (size_t i = 0; i < nsops; i++) {
    const sembuf &op = call.args.semop_args.sops[i];

    if (op.sem_num != sops[i].sem_num || op.sem_op != sops[i].sem_op || op.sem_flg != sops[i].sem_flg) {
      std::stringstream ss;
      ss << "[MOCK] semop sembuf[" << i << "] mismatch: called with sem_num=" << sops[i].sem_num
         << " sem_op=" << sops[i].sem_op << " sem_flg=" << std::hex << sops[i].sem_flg
         << " but expected sem_num=" << op.sem_num << " sem_op=" << op.sem_op << " sem_flg=" << std::hex << op.sem_flg;
      throw MockFailure(ss.str());
    }
  }

  return call.return_value;
}

extern "C" int semctl(int semid, int semnum, int cmd, ...) {
  va_list ap;
  va_start(ap, cmd);
  semun arg = va_arg(ap, semun);
  va_end(ap);

  MockCall call = pop_call(MOCK_SEMCTL);

  if (call.args.semctl_args.semid != semid || call.args.semctl_args.semnum != semnum ||
      call.args.semctl_args.cmd != cmd) {
    std::stringstream ss;
    ss << "[MOCK] semctl args mismatch: called with semid=" << semid << " semnum=" << semnum << " cmd=" << cmd
       << " but expected semid=" << call.args.semctl_args.semid << " semnum=" << call.args.semctl_args.semnum
       << " cmd=" << call.args.semctl_args.cmd;
    throw MockFailure(ss.str());
  }

  if (cmd == SETVAL) {
    // For SETVAL, the argument is passed directly as an int
    if (arg.val != call.args.semctl_args.arg.val) {
      std::stringstream ss;
      ss << "[MOCK] semctl SETVAL value mismatch: called with val=" << arg.val
         << " but expected val=" << call.args.semctl_args.arg.val;
      throw MockFailure(ss.str());
    }
  }

  return call.return_value;
}

extern "C" key_t ftok(const char *pathname, int proj_id) {
  MockCall call = pop_call(MOCK_FTOK);

  if (strcmp(call.args.ftok_args.pathname, pathname) != 0 || call.args.ftok_args.proj_id != proj_id) {
    std::stringstream ss;
    ss << "[MOCK] ftok args mismatch: called with pathname=" << pathname << " proj_id=" << proj_id
       << " but expected pathname=" << call.args.ftok_args.pathname << " proj_id=" << call.args.ftok_args.proj_id;
    throw MockFailure(ss.str());
  }

  return call.return_value;
}