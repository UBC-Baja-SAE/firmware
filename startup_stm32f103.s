#include <sys/stat.h>
#include <sys/types.h>

void _exit(int status) { while (1) {} }
int _kill(int pid, int sig) { return -1; }
int _getpid(void) { return 1; }
int _sbrk(int incr) { return -1; }
int _write(int fd, char *ptr, int len) { return len; }
int _read(int fd, char *ptr, int len) { return 0; }
int _close(int fd) { return -1; }
int _fstat(int fd, struct stat *st) { return 0; }
int _isatty(int fd) { return 1; }
int _lseek(int fd, int ptr, int dir) { return 0; }
