#pragma once

#include <queue>
#include <sys/ipc.h>
#include <sys/sem.h>

#ifdef _SEM_SEMUN_UNDEFINED
union semun {
  int val;               /* Value for SETVAL */
  struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
  unsigned short *array; /* Array for GETALL, SETALL */
  struct seminfo *__buf; /* Buffer for IPC_INFO (Linux-specific) */
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { MOCK_SEMGET, MOCK_SEMOP, MOCK_SEMCTL, MOCK_FTOK } MockSyscall;

typedef struct {
  MockSyscall syscall;
  int return_value;
  int errno_value;
  union {
    struct {
      key_t key;
      int nsems;
      int semflg;
    } semget;

    struct {
      int semid;
      const struct sembuf *sops;
      size_t nsops;
    } semop;

    struct {
      int semid;
      int semnum;
      int cmd;
      semun arg;
    } semctl;

    struct {
      const char *pathname;
      int proj_id;
    } ftok_args;
  } args;

} MockCall;

void mock_push_expected_call(MockCall call);
void mock_reset(void);

#ifdef __cplusplus
}
#endif