#include "semaphore-sysv.h"

#include <cerrno>
// #include <errnoname.h>
#include <sys/sem.h>
#include <system_error>

#define OPERATION_COUNTER 0
#define REF_COUNT 1
#define SEMAPHORES 2

SemaphoreV *SemaphoreV::create(Token &key, int mode, int value) {
  int semid;

  mode &= 0x1FF;
  do {
    semid = semget(*key, SEMAPHORES, mode | IPC_CREAT | IPC_EXCL);
    if (semid != -1) {
      // set the initial value
      semun arg;
      arg.val = value;
      if (semctl(semid, OPERATION_COUNTER, SETVAL, arg) == -1) {
        throw std::system_error(errno, std::system_category(), "semctl");
      }
    } else if (errno == EEXIST) {
      // the next call to semget can fail if there is a race and another process/thread removed the semaphore
      // if that happens, go around again and attempt to create it
      semid = semget(*key, 0, 0);
      if (semid != -1) {
        struct sembuf op;
        op.sem_num = REF_COUNT;
        op.sem_op = 1;
        op.sem_flg = SEM_UNDO;
        while (semop(semid, &op, 1) == -1) {
          if (errno != EINTR) {
            throw std::system_error(errno, std::system_category(), "semop");
          }
        }
      } else if (errno != ENOENT) {
        throw std::system_error(errno, std::system_category(), "semget");
      }
    } else {
      throw std::system_error(errno, std::system_category(), "semget");
    }
  } while (semid == -1); // the sem got unlinked in a race
  return new SemaphoreV(semid);
}

SemaphoreV *SemaphoreV::createExclusive(Token &key, int mode, int value) {
  int semid;

  mode &= 0x1FF;
  do {
    semid = semget(*key, SEMAPHORES, mode | IPC_CREAT | IPC_EXCL);
    if (semid != -1) {
      // set the initial value
      semun arg;
      arg.val = value;
      if (semctl(semid, OPERATION_COUNTER, SETVAL, arg) == -1) {
        // might want to close the sem?
        throw std::system_error(errno, std::system_category(), "semctl");
      }
    } else {
      throw std::system_error(errno, std::system_category(), "semget");
    }
  } while (semid == -1);
  return new SemaphoreV(semid);
}

SemaphoreV *SemaphoreV::open(Token &key) {
  int semid = semget(*key, SEMAPHORES, 0);
  if (semid == -1) {
    throw std::system_error(errno, std::system_category(), "semget");
  }
  struct sembuf op;
  op.sem_num = REF_COUNT;
  op.sem_op = 1;
  op.sem_flg = SEM_UNDO;
  while (semop(semid, &op, 1) == -1) {
    if (errno != EINTR) {
      throw std::system_error(errno, std::system_category(), "semop");
    }
  }
  return new SemaphoreV(semid);
}

void SemaphoreV::unlink(Token &key) {
  int semid = semget(*key, SEMAPHORES, 0);
  if (semid == -1) {
    throw std::system_error(errno, std::system_category(), "semget");
  }
  if (semctl(semid, 0, IPC_RMID) == -1) {
    throw std::system_error(errno, std::system_category(), "semctl");
  }
}

int SemaphoreV::valueOf() {
  const int result = semctl(semid, OPERATION_COUNTER, GETVAL);
  if (result != -1) {
    return result;
  }
  throw std::system_error(errno, std::system_category(), "semctl");
}

void SemaphoreV::wait() {
  struct sembuf op;
  op.sem_num = OPERATION_COUNTER;
  op.sem_op = -1;
  op.sem_flg = SEM_UNDO;
  while (semop(semid, &op, 1) == -1) {
    if (errno != EINTR) {
      throw std::system_error(errno, std::system_category(), "semop");
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
      throw std::system_error(errno, std::system_category(), "semop");
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
      throw std::system_error(errno, std::system_category(), "semop");
    }
  }
}

void SemaphoreV::op(int value) {
  struct sembuf op;
  op.sem_num = OPERATION_COUNTER;
  op.sem_op = value;
  op.sem_flg = SEM_UNDO;
  while (semop(semid, &op, 1) == -1) {
    if (errno != EINTR) {
      throw std::system_error(errno, std::system_category(), "semop");
    }
  }
}

void SemaphoreV::close() {
  struct sembuf op;
  op.sem_num = REF_COUNT;
  op.sem_op = -1;
  op.sem_flg = IPC_NOWAIT | SEM_UNDO;
  while (semop(semid, &op, 1) == -1) {
    if (errno == EAGAIN) {
      if (semctl(semid, 0, IPC_RMID) == -1) {
        throw std::system_error(errno, std::system_category(), "semctl");
      } else {
        break;
      }
    } else if (errno != EINTR) {
      throw std::system_error(errno, std::system_category(), "semop");
    }
  }
  semid = -1;
}
