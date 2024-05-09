#include <sys/file.h>
#include <sys/errno.h>
#include "errnoname/errnoname.h"


class Flock {
public:
  static void share(int fd);
  static void exclusive(int fd);
  static bool shareNB(int fd);
  static bool exclusiveNB(int fd);
  static bool unlock(int fd);
};