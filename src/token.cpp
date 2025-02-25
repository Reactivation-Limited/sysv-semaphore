#include "token.h"
#include <cerrno>
#include <system_error>

Token::Token(const char *path, char id) {
  key = ftok(path, id);
  if (key == -1) {
    throw std::system_error(errno, std::system_category(), "ftok");
  }
}

key_t Token::operator*() { return key; }

int Token::valueOf() { return (int)key; }
