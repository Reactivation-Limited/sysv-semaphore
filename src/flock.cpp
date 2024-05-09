#include <sys/file.h>
#include <sys/errno.h>
#include "errnoname/errnoname.h"
#include "flock.h"

void Flock::share(int fd) {
  if(flock(fd, LOCK_SH) == 0) {
    return;
  }
  throw errnoname(errno);
};

bool Flock::shareNB(int fd) {
  if(flock(fd, LOCK_SH | LOCK_NB) == 0) {
    return true;
  }
  if(errno == EWOULDBLOCK) {
    return false;
  }
  throw errnoname(errno);
};

void Flock::exclusive(int fd) {
  if(flock(fd, LOCK_EX) == 0) {
    return;
  }
  throw errnoname(errno);
};

bool Flock::exclusiveNB(int fd) {
  if(flock(fd, LOCK_EX | LOCK_NB) == 0) {
    return true;
  }
  if(errno == EWOULDBLOCK) {
    return false;
  }
  throw errnoname(errno);
};

bool Flock::unlock(int fd) {
  if(flock(fd, LOCK_UN) == 0) {
    return true;
  }
  throw errnoname(errno);
};
