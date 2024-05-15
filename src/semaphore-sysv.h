class SemaphoreV {
  int semid;

  SemaphoreV(int s) : semid(s){};

public:
  static SemaphoreV *createExclusive(const char *path, int mode, int value);
  static SemaphoreV *create(const char *path, int mode, int value);
  static SemaphoreV *open(const char *path);
  static void unlink(const char *path);

  ~SemaphoreV();
  void wait();
  bool trywait();
  void post();
  int valueOf();
};
