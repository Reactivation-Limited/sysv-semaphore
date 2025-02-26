class Flock {
public:
  static void share(int fd);
  static void exclusive(int fd);
  static bool tryShare(int fd);
  static bool tryExclusive(int fd);
  static void unlock(int fd);
};