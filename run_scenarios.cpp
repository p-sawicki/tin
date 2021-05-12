#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

const char *const CERT_DEFAULT = "cert.pem", *const KEY_DEFAULT = "key.pem";
const int MIN_DEFAULT = 1, MAX_DEFAULT = 101, STEP_DEFAULT = 10,
          REPEAT_DEFAULT = 1;

void usage(const char *name) {
  std::cout << "Usage:\n"
            << name
            << " [--help] [--cert PATH] [--key PATH] [--min N] [--max N]"
               " [--step N] [--repeat N]\nOptions:\n"
            << "--help - print this message and quit\n"
            << "--cert PATH - path to certificate file for servers (default: "
            << CERT_DEFAULT << ")\n"
            << "--key PATH - path to private key file for servers (default: "
            << KEY_DEFAULT << ")\n"
            << "--min N - number of clients to launch in first run (default: "
            << MIN_DEFAULT << ")\n"
            << "--max N - number of clients to launch in last run (default: "
            << MAX_DEFAULT << ")\n"
            << "--step N - increase in number of clients to launch in each run "
               "(default: "
            << STEP_DEFAULT << ")\n"
            << "--repeat N - number of times to run all scenarios (default: "
            << REPEAT_DEFAULT << ")\n";
  exit(0);
}

void parseArgs(int argc, char **argv, const char *cert, const char *key,
               int &min, int &max, int &step, int &repeat) {
  for (int i = 1; i < argc; ++i) {
    if (!strcmp("--help", argv[i])) {
      usage(argv[0]);
    } else if (i + 1 < argc) {
      if (!strcmp("--cert", argv[i])) {
        cert = argv[++i];
      } else if (!strcmp("--key", argv[i])) {
        key = argv[++i];
      } else if (!strcmp("--min", argv[i])) {
        min = atoi(argv[++i]);
      } else if (!strcmp("--max", argv[i])) {
        max = atoi(argv[++i]);
      } else if (!strcmp("--step", argv[i])) {
        step = atoi(argv[++i]);
      } else if (!strcmp("--repeat", argv[i])) {
        repeat = atoi(argv[++i]);
      } else {
        usage(argv[0]);
      }
    } else {
      usage(argv[0]);
    }
  }
}

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

template <class T, class... U> int _execl(T path, U &&...args) {
  int res = execl(path, path, std::forward<U>(args)..., nullptr);
  if (res) {
    error(__func__);
  }
  return res;
}

std::vector<pid_t> startServers(const char *cert, const char *key) {
  std::cout << "Starting servers\n";

  pid_t pico = _fork();
  if (pico == 0) {
    _execl("build/picoquic_impl/pico_server", "4433", cert, key);
  }

  // start msquic server
  // start mvfst server
  // start tcp server

  // run loggers for servers

  std::cout << "All servers started\n";
  return {pico}; // return {pico, msquic, mvfst, tcp};
}

void picoquic(int clients) {
  std::cout << "\t\tRunning picoquic clients\n";

  std::vector<const char *> scenarios{"1", "2", "3", "4"};
  for (const char *scenario : scenarios) {
    std::cout << "\t\t\tScenario " << scenario << "\n";
    std::vector<pid_t> clientPIDs;

    for (int i = 0; i < clients; ++i) {
      pid_t clientPID = _fork();
      if (clientPID == 0) {
        _execl("build/picoquic_impl/pico_client", "localhost", "4433",
               scenario);
      }
      clientPIDs.push_back(clientPID);
    }

    // run logger for clients

    for (int i = 0; i < clients; ++i) {
      _wait();
    }
  }
  std::cout << "\t\tAll scenarios with picoquic finished\n";
}

int main(int argc, char **argv) {
  const char *cert = CERT_DEFAULT, *key = KEY_DEFAULT;
  int min = MIN_DEFAULT, max = MAX_DEFAULT, step = STEP_DEFAULT,
      repeat = REPEAT_DEFAULT;

  parseArgs(argc, argv, cert, key, min, max, step, repeat);

  auto serverPIDs = startServers(cert, key);

  for (int i = 0; i < repeat; ++i) {
    std::cout << "Run #" << i + 1 << "\n";

    for (int j = min; j <= max; j += step) {
      std::cout << "\tStarting scenarios with " << j << " clients\n";

      picoquic(j);

      // msquic(j);

      // mvfst(j);

      // tcp(j);
    }
  }

  std::cout << "All runs finished, killing servers\n";

  for (pid_t pid : serverPIDs) {
    if (kill(pid, SIGKILL) < 0) {
      error("kill");
    }
  }

  std::cout << "Goodbye\n";
}