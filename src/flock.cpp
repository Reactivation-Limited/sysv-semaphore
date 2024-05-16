#include "flock.h"
#include <errnoname.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <system_error>

void Flock::share(int fd) {
  if (flock(fd, LOCK_SH) == 0) {
    return;
  }
  throw errnoname(errno);
};

bool Flock::shareNB(int fd) {
  if (flock(fd, LOCK_SH | LOCK_NB) == 0) {
    return true;
  }
  if (errno == EWOULDBLOCK) {
    return false;
  }
  throw errnoname(errno);
};

void Flock::exclusive(int fd) {
  if (flock(fd, LOCK_EX) == 0) {
    return;
  }
  throw errnoname(errno);
};

bool Flock::exclusiveNB(int fd) {
  if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
    return true;
  }
  if (errno == EWOULDBLOCK) {
    return false;
  }
  throw errnoname(errno);
};

void Flock::unlock(int fd) {
  if (flock(fd, LOCK_UN) == 0) {
    return;
  }
  throw std::system_error(errno, std::system_category(), "flock");
};
