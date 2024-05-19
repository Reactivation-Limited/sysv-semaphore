#include "errnoname.h"
#include <cerrno>

class Flock {
public:
  static void share(int fd);
  static void exclusive(int fd);
  static bool shareNB(int fd);
  static bool exclusiveNB(int fd);
  static void unlock(int fd);
};