// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 15.05.2021
// Autor: Piotr Sawicki

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