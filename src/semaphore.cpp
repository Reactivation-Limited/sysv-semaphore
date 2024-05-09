#include <semaphore.h>
#include <string.h>
#include <sys/errno.h>
#include <stdio.h>
#include <cstring>
#include "errnoname/errnoname.h"
#include "semaphore.h"

void Semaphore::open(const char *name, unsigned int value = 1, int oflag = o_creat, int mode = 0600) {
  n = new char[256];
  n[255] = '\0';
  std::strncpy(n, name, 255);
  do {
    s = sem_open(name, oflag, mode, value); 
    if(s != SEM_FAILED) {
      return;
    }
  } while(errno == EINTR);
  throw errnoname(errno);
}


void Semaphore::wait() {
  int r;
  do {
    r = sem_wait(s); 
  } while(r == -1 && errno == EINTR);
  if(r != 0) {
    throw errnoname(errno);
  }
}

bool Semaphore::trywait() {
  do {
    if(sem_trywait(s) == 0) {
      return true;
    }
    if(errno == EAGAIN){
      return false;
    }
  } while(errno == EINTR);
  throw errnoname(errno);
}

void Semaphore::post() {
  if(sem_post(s) == -1) {
    throw errnoname(errno);
  }
}

void Semaphore::close() {
  if(sem_close(s) == -1) {
    throw errnoname(errno);
  }
  s = NULL;
  delete n;
  n = NULL;
}


void Semaphore::unlink() {
  if(sem_unlink(n) == -1) {
    throw errnoname(errno);
  }
  s = NULL;
  delete n;
  n = NULL;
}
