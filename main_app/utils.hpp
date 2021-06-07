// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 15.05.2021
// Autor: Piotr Sawicki

#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <mqueue.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

void error(const char *func);
pid_t _fork();
pid_t _wait();

template <class T, class... U> int _execl(T path, U &&...args) {
  int res = execl(path, path, std::forward<U>(args)..., nullptr);
  if (res) {
    error(__func__);
  }
  return res;
}

#endif // UTILS_HPP