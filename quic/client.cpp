#include <glog/logging.h>
#include <fizz/crypto/Utils.h>
#include <folly/init/Init.h>
#include <folly/portability/GFlags.h>
#include "Client.h"
#include <iostream>
#include <fstream>

DEFINE_string(host, "::1", "Echo server hostname/IP");
DEFINE_int32(port, 6666, "Echo server port");
DEFINE_string(mode, "server", "Mode to run in: 'client' or 'server'");
DEFINE_string(emode, "quiet", "Mode to run in: 'quiet' or 'no-delay'");

using namespace quic;
using namespace fizz;

int delay(int argc, char **argv, int scenario) {
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--no-delay")) {
      return 0;
    }
  }
  int sleepTime = (scenario+1)/2;
  srand(getpid());
  sleep(rand() % (10)+20);
  return 1;
}

int main(int argc, char* argv[]) {
#if FOLLY_HAVE_LIBGFLAGS
  // Enable glog logging to stderr by default.
  gflags::SetCommandLineOptionWithMode(
      "logtostderr", "1", gflags::SET_FLAGS_DEFAULT);
#endif
  //gflags::ParseCommandLineFlags(&argc, &argv, false);
  folly::Init init(&argc, &argv);
  fizz::CryptoUtils::init();
  
  const std::string& server_host = argv[1];
  uint16_t server_port = atoi(argv[2]);
  int scenario = atoi(argv[3]);
  std::string flag = argv[4];
  int ifDelay = delay(argc, argv, scenario);

  Client client(FLAGS_host, FLAGS_port);
  client.start(ifDelay, scenario); 

  return 0;
}

