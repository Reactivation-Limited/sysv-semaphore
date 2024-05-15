#include "semaphore-posix.h"

#include <errnoname.h>
#include <stddef.h>
#include <sys/errno.h>

SemaphoreP::~SemaphoreP() {
  if (s != SEM_FAILED) {
    sem_close(s);
  }
  s = NULL;
};

SemaphoreP *SemaphoreP::open(const char *name) {
  do {
    auto s = sem_open(name, 0);
    if (s != SEM_FAILED) {
      return new SemaphoreP(s);
    }
  } while (errno == EINTR);
  throw errnoname(errno);
}

SemaphoreP *SemaphoreP::createP(const char *name, int oflags, int mode, unsigned int value) {
  do {
    auto s = sem_open(name, oflags, mode, value);
    if (s != SEM_FAILED) {
      return new SemaphoreP(s);
    }
  } while (errno == EINTR);
  throw errnoname(errno);
}

SemaphoreP *SemaphoreP::createExclusive(const char *name, int mode, unsigned int value)

{
  return createP(name, O_CREAT | O_EXCL, mode, value);
}

SemaphoreP *SemaphoreP::create(const char *name, int mode, unsigned int value)

{
  return createP(name, O_CREAT, mode, value);
}

void SemaphoreP::wait() {
  if (s == SEM_FAILED) {
    throw "already closed";
  }
  int r;
  do {
    r = sem_wait(s);
  } while (r == -1 && errno == EINTR);
  if (r != 0) {
    throw errnoname(errno);
  }
}

bool SemaphoreP::trywait() {
  if (s == SEM_FAILED) {
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
  throw errnoname(errno);
}

void SemaphoreP::post() {
  if (sem_post(s) == -1) {
    throw errnoname(errno);
  }
}

void SemaphoreP::close() {
  if (s != SEM_FAILED && sem_close(s) == -1) {
    throw errnoname(errno);
  }
  s = SEM_FAILED;
}

void SemaphoreP::unlink(const char *name) {
  if (name == NULL) {
    return;
  }
  if (sem_unlink(name) == -1) {
    throw errnoname(errno);
  }
}
