#include <semaphore.h>

const int o_creat = O_CREAT;
const int o_excl = O_EXCL;
const int o_ur = 0x100;
const int o_uw = 0x80;
const int o_gr = 0x20;
const int o_gw = 0x10;
const int o_or = 0x04;
const int o_ow = 0x02;

class Semaphore {
  char *n = NULL;
  sem_t *s = NULL;
public:
  Semaphore();
  ~Semaphore();
  void open(const char *name,  unsigned int value, int oflag, int mode);
  void wait();
  bool trywait();
  void post();
  void close();
  void unlink();
};

void unlink(const char *name);
