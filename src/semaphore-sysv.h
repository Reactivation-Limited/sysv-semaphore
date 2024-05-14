#include <stddef.h>
#include <sys/sem.h>

class SemaphoreV {
  int semid;

  SemaphoreV(int s) : semid(s){};

public:
  static SemaphoreV *create(const char *path, int value);
  static SemaphoreV *open(const char *path);
  static void unlink(const char *name);

  ~SemaphoreV();
  void wait();
  bool trywait();
  void post();
};
