#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>

class Semaphore {
  char *n;
  sem_t *s;

public:
  Semaphore();
  ~Semaphore(){};
  void open(const char *name, unsigned int value = 1, int oflag = O_CREAT,
            int mode = 0600);
  void wait();
  bool trywait();
  void post();
  void close();
  void unlink();
};
