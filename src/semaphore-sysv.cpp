#include "semaphore-sysv.h"

#include <cerrno>
#include <sys/sem.h>
#include <system_error>

#ifdef _SEM_SEMUN_UNDEFINED
union semun {
  int val;               /* Value for SETVAL */
  struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
  unsigned short *array; /* Array for GETALL, SETALL */
  struct seminfo *__buf; /* Buffer for IPC_INFO (Linux-specific) */
};
#endif

#define OPERATION_COUNTER 0
#define REF_COUNT 1
#define SEMAPHORES 2

SemaphoreV *SemaphoreV::create(Token &key, int mode, int value) {
  int semid;

  mode &= 0x1FF;
  do {
    // use IPC_CREAT to determine if the initial value should be set
    semid = semget(*key, SEMAPHORES, mode | IPC_CREAT | IPC_EXCL);
    if (semid != -1) {
      // set the initial value
      semun arg;
      arg.val = value;
      if (semctl(semid, OPERATION_COUNTER, SETVAL, arg) == -1) {
        throw std::system_error(errno, std::system_category(), "semctl");
      }
      return new SemaphoreV(semid);
    } else if (errno != EEXIST) {
      throw std::system_error(errno, std::system_category(), "semget");
    } else {
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
        return new SemaphoreV(semid);
      } else if (errno == ENOENT) {
        continue;
      } else {
        throw std::system_error(errno, std::system_category(), "semget");
      }
    }
  } while (true); // a race is possible with another process, so loop until one of the semget calls works
  return new SemaphoreV(semid);
}

SemaphoreV *SemaphoreV::createExclusive(Token &key, int mode, int value) {
  int semid;

  mode &= 0777;
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

unsigned SemaphoreV::valueOf() {
  const int result = semctl(semid, OPERATION_COUNTER, GETVAL);
  if (result != -1) {
    return result;
  }
  throw std::system_error(errno, std::system_category(), "semctl");
}

unsigned SemaphoreV::refs() {
  const int result = semctl(semid, REF_COUNT, GETVAL);
  if (result != -1) {
    return result;
  }
  throw std::system_error(errno, std::system_category(), "semctl");
}

void SemaphoreV::wait() { wait(1); }

void SemaphoreV::wait(unsigned value) {
  struct sembuf op;
  op.sem_num = OPERATION_COUNTER;
  op.sem_op = -value;
  op.sem_flg = SEM_UNDO;
  while (semop(semid, &op, 1) == -1) {
    if (errno != EINTR) {
      throw std::system_error(errno, std::system_category(), "semop");
    }
  }
}

bool SemaphoreV::trywait() { return trywait(1); }

bool SemaphoreV::trywait(unsigned value) {
  struct sembuf op;
  op.sem_num = OPERATION_COUNTER;
  op.sem_op = -value;
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

void SemaphoreV::post() { post(1); }

void SemaphoreV::post(unsigned value) {
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
    if (errno == EAGAIN) { // indicates the REF_COUNT is 0
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

SemaphoreV::~SemaphoreV() {
  if (semid == -1) {
    return;
  }
  try {
    close();
  } catch (...) {
    // Destructor should never throw - silently ignore cleanup errors
  }
}
