#include "token.h"
#include <sys/ipc.h>

class SemaphoreV {
  int semid;

  SemaphoreV(int s) : semid(s){};

public:
  static SemaphoreV *createExclusive(Token &key, int mode, int value);
  static SemaphoreV *create(Token &key, int mode, int value);
  static SemaphoreV *open(Token &key);
  static void unlink(Token &key);

  ~SemaphoreV();
  void wait();
  bool trywait();
  void post();
  void op(int value);
  int valueOf();
  void close();
};
