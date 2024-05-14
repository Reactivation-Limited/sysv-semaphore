#include <semaphore.h>

class Semaphore {
  sem_t *s;

  Semaphore(sem_t *s) : s(s){};

  static Semaphore *create(const char *name, int oflags, int mode, unsigned int value);

public:
  ~Semaphore();
  bool valueOf() { return s != SEM_FAILED; };
  void wait();
  bool trywait();
  void post();
  void close();

  static Semaphore *createShared(const char *name, int mode, unsigned int value);
  static Semaphore *createExclusive(const char *name, int mode, unsigned int value);
  static Semaphore *open(const char *name);
  static void unlink(const char *name);
};
