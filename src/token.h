#include <sys/ipc.h>

class Token {
public:
  key_t key;

public:
  Token(const char *path, char id);
  key_t operator*();
  int valueOf();
};
