#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>

class Semaphore {
  char *n;
  sem_t *s;
  // create(const char *name, int oflags, int mode, unsigned int value);
public:
  Semaphore();
  ~Semaphore(){};
  void create(const char *name, int mode, unsigned int value);
  void createExclusive(const char *name, int mode, unsigned int value);
  void open(const char *name);
  void wait();
  bool trywait();
  void post();
  void close();
  void unlink();
};
