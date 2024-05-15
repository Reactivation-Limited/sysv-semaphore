#include "semaphore-sysv.h"

#include <errnoname.h>
#include <sys/errno.h>
#include <sys/sem.h>

#include <stdio.h>

#define OPERATION_COUNTER 0
#define REF_COUNT 1
#define SEMAPHORES 2

auto tok(const char *path) { return ftok(path, 0); }

SemaphoreV *SemaphoreV::create(const char *path, int mode, int value) {
  auto key = tok(path);
  int semid;

  mode &= 0x1FF;
  do {
    semid = semget(key, SEMAPHORES, mode | IPC_CREAT | IPC_EXCL);
    if (semid != -1) {
      // set the initial value
      semun arg;
      arg.val = value;
      if (semctl(semid, OPERATION_COUNTER, SETVAL, arg) == -1) {
        // might want to close the sem?
        throw errnoname(errno);
      }
    } else if (errno == EEXIST) {
      // open the existing sem. can fail if another process unlinked the sem before the second call to semget
      semid = semget(key, 0, 0);
      if (semid != -1) {
        struct sembuf op;
        op.sem_num = REF_COUNT;
        op.sem_op = 1;
        op.sem_flg = SEM_UNDO;
        while (semop(semid, &op, 1) == -1) {
          if (errno != EINTR) {
            throw errnoname(errno);
          }
        }
      } else if (errno != ENOENT) {
        throw errnoname(errno);
      }
    } else {
      throw errnoname(errno);
    }
  } while (semid == -1); // the sem got unlinked in a race
  return new SemaphoreV(key, semid);
}

SemaphoreV *SemaphoreV::createExclusive(const char *path, int mode, int value) {
  auto key = tok(path);
  int semid;

  mode &= 0x1FF;
  do {
    semid = semget(key, SEMAPHORES, mode | IPC_CREAT | IPC_EXCL);
    if (semid != -1) {
      // set the initial value
      semun arg;
      arg.val = value;
      if (semctl(semid, OPERATION_COUNTER, SETVAL, arg) == -1) {
        // might want to close the sem?
        throw errnoname(errno);
      }
    } else {
      throw errnoname(errno);
    }
  } while (semid == -1);
  return new SemaphoreV(key, semid);
}

SemaphoreV *SemaphoreV::open(const char *path) {
  auto key = tok(path);

  int semid = semget(key, SEMAPHORES, 0);
  if (semid == -1) {
    throw errnoname(errno);
  }
  struct sembuf op;
  op.sem_num = REF_COUNT;
  op.sem_op = 1;
  op.sem_flg = SEM_UNDO;
  while (semop(semid, &op, 1) == -1) {
    if (errno != EINTR) {
      throw errnoname(errno);
    }
  }

  return new SemaphoreV(key, semid);
}

void SemaphoreV::unlink(const char *path) {
  auto key = tok(path);
  int semid;

  semid = semget(key, 1, 0);
  if (semid == -1) {
    throw errnoname(errno);
  }
  if (semctl(semid, 0, IPC_RMID) == -1) {
    throw errnoname(errno);
  }
}

int SemaphoreV::valueOf() {
  const int result = semctl(semid, OPERATION_COUNTER, GETVAL);
  if (result != -1) {
    return result;
  }
  throw errnoname(errno);
}

void SemaphoreV::wait() {
  struct sembuf op;
  op.sem_num = OPERATION_COUNTER;
  op.sem_op = -1;
  op.sem_flg = SEM_UNDO;
  while (semop(semid, &op, 1) == -1) {
    if (errno != EINTR) {
      throw errnoname(errno);
    }
  }
}

bool SemaphoreV::trywait() {
  struct sembuf op;
  op.sem_num = OPERATION_COUNTER;
  op.sem_op = -1;
  op.sem_flg = SEM_UNDO | IPC_NOWAIT;
  while (semop(semid, &op, 1) == -1) {
    if (errno == EAGAIN) {
      return false;
    }
    if (errno != EINTR) {
      throw errnoname(errno);
    }
  }
  return true;
}

void SemaphoreV::post() {
  struct sembuf op;
  op.sem_num = OPERATION_COUNTER;
  op.sem_op = 1;
  op.sem_flg = SEM_UNDO;
  while (semop(semid, &op, 1) == -1) {
    if (errno != EINTR) {
      throw errnoname(errno);
    }
  }
}

void SemaphoreV::close() {
  struct sembuf op;
  op.sem_num = REF_COUNT;
  op.sem_op = -1;
  op.sem_flg = IPC_NOWAIT;
  while (semop(semid, &op, 1) == -1) {
    if (errno == EAGAIN) {
      if (semctl(semid, 0, IPC_RMID) == -1) {
        throw errnoname(errno);
      } else {
        break;
      }
    } else if (errno != EINTR) {
      throw errnoname(errno);
    }
  }
  semid = -1;
}
