#include "semaphore.h"
#include <cstring>
#include <errnoname.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>

Semaphore::Semaphore() {
  s = SEM_FAILED;
  n = NULL;
};

void Semaphore::open(const char *name, unsigned int value, int oflag,
                     int mode) {
  if (s == SEM_FAILED) {
    throw "semaphore is already open";
  }
  n = new char[31];
  n[30] = '\0';
  std::strncpy(n, name, 30);
  do {
    s = sem_open(name, oflag, mode, value);
    if (s != SEM_FAILED) {
      return;
    }
  } while (errno == EINTR);
  throw errnoname(errno);
}

void Semaphore::wait() {
  int r;
  do {
    r = sem_wait(s);
  } while (r == -1 && errno == EINTR);
  if (r != 0) {
    throw errnoname(errno);
  }
}

bool Semaphore::trywait() {
  do {
    if (sem_trywait(s) == 0) {
      return true;
    }
    if (errno == EAGAIN) {
      return false;
    }
  } while (errno == EINTR);
  throw errnoname(errno);
}

void Semaphore::post() {
  if (sem_post(s) == -1) {
    throw errnoname(errno);
  }
}

void Semaphore::close() {
  if (sem_close(s) == -1) {
    throw errnoname(errno);
  }
  s = SEM_FAILED;
}

void Semaphore::unlink() {
  if (sem_unlink(n) == -1) {
    throw errnoname(errno);
  }
  s = SEM_FAILED;
  delete n;
  n = NULL;
}
