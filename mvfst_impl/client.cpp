// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 06.07.2021
// Autor: Anna Pawlus

#include "Client.h"
#include <fizz/crypto/Utils.h>
#include <folly/init/Init.h>
#include <folly/portability/GFlags.h>
#include <fstream>
#include <glog/logging.h>
#include <iostream>

DEFINE_string(host, "::1", "Server hostname/IP");
DEFINE_int32(port, 4436, "Server port");
DEFINE_string(emode, "quiet", "Mode to run in: 'quiet' or 'no-delay'");

using namespace quic;
using namespace fizz;

int delay(int argc, char **argv, int scenario) {
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--no-delay")) {
      return 0;
    }
  }
  int waitTime = (unsigned int[]) {60, 60, 6, 6}[scenario - 1];
  srand(getpid());
  sleep(rand() % (60/waitTime));
  return 1;
}

int main(int argc, char *argv[]) {
#if FOLLY_HAVE_LIBGFLAGS
  // Enable glog logging to stderr by default.
  gflags::SetCommandLineOptionWithMode(
      "logtostderr", "1", gflags::SET_FLAGS_DEFAULT);
#endif
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  folly::Init init(&argc, &argv);
  fizz::CryptoUtils::init();

  const std::string &server_host = argv[1];
  uint16_t server_port = atoi(argv[2]);
  int scenario = atoi(argv[3]);
  int ifDelay = delay(argc, argv, scenario);

  Client client(FLAGS_host, FLAGS_port);

  client.initQuicClient();
  client.runQuicClient(ifDelay, scenario);

  return 0;
}