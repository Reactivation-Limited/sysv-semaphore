#include <semaphore.h>

class SemaphoreP {
  sem_t *s;

  SemaphoreP(sem_t *s) : s(s){};

  static SemaphoreP *createP(const char *name, int oflags, int mode, unsigned int value);

public:
  ~SemaphoreP();
  void wait();
  bool trywait();
  void post();
  void close();

  // dump mode, just UR|UW
  static SemaphoreP *createExclusive(const char *name, int mode, unsigned int value);
  static SemaphoreP *create(const char *name, int mode, unsigned int value);
  static SemaphoreP *open(const char *name);
  static void unlink(const char *name);
};
