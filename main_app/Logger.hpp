// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 15.05.2021
// Autor: Piotr Sawicki

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "utils.hpp"
#include <string_view>

class Logger {
  const std::string_view inName, outName;
  mqd_t in, out;

  mqd_t _mq_open(const char *name, int oflag) const;
  void _mq_send(mqd_t q, const char *msg) const;
  void _mq_receive(mqd_t q) const;
  void _mq_unlink(const char *name) const;

public:
  Logger(const char *qIn, const char *qOut);
  void logPID(pid_t pid) const;
  void remove(int n) const;
  ~Logger();
};

#endif // LOGGER_HPP