// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 12.05.2021
// Autor: Marcin Białek

#include <gtest/gtest.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

TEST(LoggerTest, communication) {
  struct mq_attr attr = {0};
  attr.mq_msgsize = sizeof(int);
  attr.mq_maxmsg = 8;
  mqd_t inqueue = mq_open("/logger-test-in", O_CREAT | O_RDWR, 0777, &attr);
  mqd_t outqueue = mq_open("/logger-test-out", O_CREAT | O_RDWR, 0777, &attr);

  if (inqueue == -1 || outqueue == -1) {
    throw std::runtime_error("error creating queue");
  }

  pid_t logger = fork();
  if (logger == 0) {
    execl("../../logger/logger.py", "logger.py", "/logger-test-in",
          "/logger-test-out", nullptr);
  }

  int msg = getpid();
  mq_send(inqueue, (const char *)&msg, sizeof(msg), 0);
  mq_receive(outqueue, (char *)&msg, 4, NULL);
  EXPECT_EQ(msg, getpid());

  msg = logger;
  mq_send(inqueue, (const char *)&msg, sizeof(msg), 0);
  mq_receive(outqueue, (char *)&msg, 4, NULL);
  EXPECT_EQ(msg, logger);

  msg = -1;
  mq_send(inqueue, (const char *)&msg, sizeof(msg), 0);
  mq_receive(outqueue, (char *)&msg, 4, NULL);
  EXPECT_EQ(msg, 0);

  msg = -1;
  mq_send(inqueue, (const char *)&msg, sizeof(msg), 0);
  mq_receive(outqueue, (char *)&msg, 4, NULL);
  EXPECT_EQ(msg, 0);

  msg = -2;
  mq_send(inqueue, (const char *)&msg, sizeof(msg), 0);
  mq_receive(outqueue, (char *)&msg, 4, NULL);
  EXPECT_EQ(msg, 0);

  close(inqueue);
  close(outqueue);
}