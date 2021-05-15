#include "utils.hpp"

void error(const char *func) {
  std::cerr << func << " ERROR: " << strerror(errno) << "\n";
  exit(errno);
}

pid_t _fork() {
  pid_t res = fork();
  if (res < 0) {
    error(__func__);
  }
  return res;
}

pid_t _wait() {
  int status;
  pid_t res = wait(&status);
  if (res < 0) {
    error(__func__);
  }
  return res;
}