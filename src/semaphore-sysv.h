#include "token.h"

class SemaphoreV {
  int semid;

  SemaphoreV(int s) : semid(s){};

public:
  static SemaphoreV *createExclusive(Token &key, int mode, int value);
  static SemaphoreV *create(Token &key, int mode, int value);
  static SemaphoreV *open(Token &key);
  static void unlink(Token &key);

  void wait();
  void wait(unsigned value);
  bool trywait();
  bool trywait(unsigned value);
  void post();
  void post(unsigned value);
  unsigned valueOf();
  unsigned refs();
  void close();

  ~SemaphoreV();
};
