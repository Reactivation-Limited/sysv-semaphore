#include "semaphore.h"
#include <cstring>
#include <errnoname.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <system_error>

Semaphore::Semaphore() {
  s = SEM_FAILED;
  n = NULL;
};

void Semaphore::open(const char *name) {
  if (s != SEM_FAILED) {
    printf("already open\n");
    throw "semaphore is already open";
  }
  n = new char[31];
  n[30] = '\0';
  std::strncpy(n, name, 30);
  do {
    s = sem_open(name, 0);
    if (s != SEM_FAILED) {
      return;
    }
  } while (errno == EINTR);
  printf("%s:%d %d %s\n", __FILE__, __LINE__, errno, errnoname(errno));
  throw errnoname(errno);
}

void Semaphore::createExclusive(const char *name, int mode, unsigned int value)

{
  if (s != SEM_FAILED) {
    printf("already open\n");
    throw "semaphore is already open";
  }
  n = new char[31];
  n[30] = '\0';
  std::strncpy(n, name, 30);
  do {
    s = sem_open(name, O_CREAT | O_EXCL, mode, value);
    if (s != SEM_FAILED) {
      return;
    }
  } while (errno == EINTR);
  printf("%s:%d %d %s\n", __FILE__, __LINE__, errno, errnoname(errno));
  throw errnoname(errno);
}

void Semaphore::create(const char *name, int mode, unsigned int value)

{
  if (s != SEM_FAILED) {
    printf("already open\n");
    throw "semaphore is already open";
  }
  n = new char[31];
  n[30] = '\0';
  std::strncpy(n, name, 30);
  do {
    s = sem_open(name, O_CREAT, mode, value);
    if (s != SEM_FAILED) {
      return;
    }
  } while (errno == EINTR);
  printf("%s:%d %d %s\n", __FILE__, __LINE__, errno, errnoname(errno));
  throw errnoname(errno);
}

void Semaphore::wait() {
  if (s == SEM_FAILED) {
    printf("%s:%d %d %s\n", __FILE__, __LINE__, 0, "already closed");
    throw "already closed";
  }
  int r;
  do {
    r = sem_wait(s);
  } while (r == -1 && errno == EINTR);
  if (r != 0) {
    printf("%s:%d %d %s\n", __FILE__, __LINE__, errno, errnoname(errno));
    throw errnoname(errno);
  }
}

bool Semaphore::trywait() {
  if (s == SEM_FAILED) {
    printf("%s:%d %d %s\n", __FILE__, __LINE__, 0, "already closed");
    throw "already closed";
  }
  do {
    if (sem_trywait(s) == 0) {
      return true;
    }
    if (errno == EAGAIN) {
      return false;
    }
  } while (errno == EINTR);
  printf("%s:%d %d %s\n", __FILE__, __LINE__, errno, errnoname(errno));
  throw errnoname(errno);
}

void Semaphore::post() {
  if (s == SEM_FAILED) {
    printf("%s:%d %d %s\n", __FILE__, __LINE__, 0, "already closed");
    throw "already closed";
  }
  if (sem_post(s) == -1) {
    throw errnoname(errno);
  }
}

void Semaphore::close() {
  if (s == SEM_FAILED) {
    printf("%s:%d %d %s\n", __FILE__, __LINE__, 0, "already closed");
    return;
  }
  if (sem_close(s) == -1) {
    printf("%s:%d %d %s\n", __FILE__, __LINE__, errno, errnoname(errno));
    throw errnoname(errno);
  }
  s = SEM_FAILED;
}

void Semaphore::unlink() {
  if (n == NULL) {
    printf("%s:%d %d %s\n", __FILE__, __LINE__, 0, "already unlinked?");
    return;
  }
  if (sem_unlink(n) == -1) {
    printf("%s:%d %d %s\n", __FILE__, __LINE__, errno, errnoname(errno));
    throw errnoname(errno);
  }
  s = SEM_FAILED;
  delete n;
  n = NULL;
}
