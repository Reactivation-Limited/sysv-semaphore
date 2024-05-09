#include "errnoname.h"
#include <sys/errno.h>
#include <sys/file.h>

class Flock {
public:
  static void share(int fd);
  static void exclusive(int fd);
  static bool shareNB(int fd);
  static bool exclusiveNB(int fd);
  static bool unlock(int fd);
};