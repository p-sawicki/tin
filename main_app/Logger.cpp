// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 15.05.2021
// Autor: Piotr Sawicki

#include "Logger.hpp"

mqd_t Logger::_mq_open(const char *name, int oflag) const {
  mq_attr attr{};
  attr.mq_msgsize = sizeof(int);
  attr.mq_maxmsg = 10;

  mqd_t res = mq_open(name, O_CREAT | oflag, 0777, &attr);
  if (res < 0) {
    error(__func__);
  }
  return res;
}

void Logger::_mq_send(mqd_t q, const char *msg) const {
  if (mq_send(q, msg, sizeof(int), 0)) {
    error(__func__);
  }
}

void Logger::_mq_receive(mqd_t q) const {
  int res;
  if (mq_receive(q, (char *)&res, sizeof(int), nullptr) < 0) {
    error(__func__);
  }

  if (res < 0) {
    error("logger");
  }
}

void Logger::_mq_unlink(const char *name) const {
  if (mq_unlink(name)) {
    error(__func__);
  }
}

Logger::Logger(const char *qIn, const char *qOut) : inName(qIn), outName(qOut) {
  in = _mq_open(qIn, O_WRONLY);
  out = _mq_open(qOut, O_RDONLY);

  if (_fork() == 0) {
    _execl("logger/logger.py", "-p", "100", "-l", "logs", qIn, qOut);
  }
}

void Logger::logPID(pid_t pid) const {
  _mq_send(in, (const char *)&pid);
  _mq_receive(out);
}

void Logger::remove(int n) const {
  int pop = -1;
  for (int i = 0; i < n; ++i) {
    _mq_send(in, (const char *)&pop);
    _mq_receive(out);
  }
}

Logger::~Logger() {
  int kill = -2;
  _mq_send(in, (const char *)&kill);
  _mq_receive(out);

  _mq_unlink(inName.data());
  _mq_unlink(outName.data());
}