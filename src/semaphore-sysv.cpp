#include "semaphore-sysv.h"

#include <errnoname.h>
#include <sys/errno.h>
#include <sys/sem.h>

auto tok(const char *path) { return ftok(path, 0); }

SemaphoreV *SemaphoreV::create(const char *path, int value) {
  auto key = tok(path);
  int semid;

  do {
    semid = semget(key, 1, IPC_CREAT | IPC_EXCL | SEM_R | SEM_A);
    if (semid != -1) {
      // set the initial value
      semun arg;
      arg.val = value;
      if (semctl(semid, 0, SETVAL, arg) == -1) {
        // might want to close the sem?
        throw errnoname(errno);
      }
    } else if (errno == EEXIST) {
      // open the existing sem. can fail if another process unlinked the sem before the second call to semget
      semid = semget(key, 1, 0);
      if (semid == -1 && errno != ENOENT) {
        throw errnoname(errno);
      }
    } else {
      throw errnoname(errno);
    }
  } while (semid == -1); // the sem got unlinked in a race
  return new SemaphoreV(semid);
}

SemaphoreV *SemaphoreV::createExclusive(const char *path, int value) {
  auto key = tok(path);
  int semid;

  do {
    semid = semget(key, 1, IPC_CREAT | IPC_EXCL | SEM_R | SEM_A);
    if (semid != -1) {
      // set the initial value
      semun arg;
      arg.val = value;
      if (semctl(semid, 0, SETVAL, arg) == -1) {
        // might want to close the sem?
        throw errnoname(errno);
      }
    } else {
      throw errnoname(errno);
    }
  } while (semid == -1);
  return new SemaphoreV(semid);
}

SemaphoreV *SemaphoreV::open(const char *path) {
  auto key = tok(path);

  int semid = semget(key, 1, 0);
  if (semid == -1) {
    throw errnoname(errno);
  }
  return new SemaphoreV(semid);
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
  const int result = semctl(semid, 0, GETVAL);
  if (result != -1) {
    return result;
  }
  throw errnoname(errno);
}

void SemaphoreV::wait() {
  struct sembuf op;
  op.sem_num = 0;
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
  op.sem_num = 0;
  op.sem_op = -1;
  op.sem_flg = SEM_UNDO | IPC_NOWAIT;
  while (semop(semid, &op, 1) == -1) {
    if (errno == EINTR) {
      continue;
    }
    if (errno == EAGAIN) {
      return false;
    }
    throw errnoname(errno);
  }
  return true;
}

void SemaphoreV::post() {
  struct sembuf op;
  op.sem_num = 0;
  op.sem_op = 1;
  op.sem_flg = SEM_UNDO;
  while (semop(semid, &op, 1) == -1) {
    if (errno == EINTR) {
      continue;
    }
    throw errnoname(errno);
  }
}
