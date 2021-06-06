#include "Server.h"
#include <fizz/crypto/Utils.h>
#include <folly/init/Init.h>
#include <folly/portability/GFlags.h>
#include <glog/logging.h>

DEFINE_string(host, "::1", "Server hostname/IP");
DEFINE_int32(port, 6666, "Server port");
DEFINE_string(mode, "server", "Mode to run in: 'client' or 'server'");

using namespace quic;
using namespace fizz;

int main(int argc, char *argv[]) {
#if FOLLY_HAVE_LIBGFLAGS
// Enable glog logging to stderr by default.
//  gflags::SetCommandLineOptionWithMode(
//      "logtostderr", "1", gflags::SET_FLAGS_DEFAULT);
#endif
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  folly::Init init(&argc, &argv);
  fizz::CryptoUtils::init();

  quic::Server server(FLAGS_host, FLAGS_port);
  server.start();

  return 0;
}
