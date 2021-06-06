#include "Logger.hpp"
#include "utils.hpp"

const char *CERT_DEFAULT = "cert.pem", *KEY_DEFAULT = "key.pem",
           *LOGGER_IN = "/logger_in", *LOGGER_OUT = "/logger_out",
           *PICO_PORT = "4433", *MS_PORT = "4434",  *TCP_PORT = "4435", *MV_PORT = "4436";
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

std::vector<pid_t> startServers(const char *cert, const char *key) {
  std::cout << "Starting servers\n";

  pid_t pico = _fork();
  if (pico == 0) {
    _execl("build/picoquic_impl/pico_server", PICO_PORT, cert, key, "--quiet");
  }

  pid_t msquic = _fork();
  if (msquic == 0) {
    _execl("build/msquic_impl/ms_server", MS_PORT, cert, key, "--quiet");
  }

  // start mvfst server
  pid_t mvfst = _fork();
  if (mvfst == 0) {
    _execl("build/quic/mvserver");
  }

  // start tcp server
  pid_t tcp = _fork();
  if (tcp == 0) {
    _execl("build/tcp/tcp_server", TCP_PORT, cert, key, "--quiet");
  }

  std::cout << "All servers started\n";
  return {pico, msquic, mvfst, tcp};
}

void runClients(int nbClients, const Logger &logger, const char *name,
                const char *path, const char *port) {
  std::cout << "\t\tRunning " << name << " clients\n";

  std::vector<const char *> scenarios{"1", "2", "3", "4"};
  for (const char *scenario : scenarios) {
    std::cout << "\t\t\tScenario " << scenario << "\n";
    std::vector<pid_t> clientPIDs;

    for (int i = 0; i < nbClients; ++i) {
      pid_t clientPID = _fork();
      if (clientPID == 0) {
        _execl(path, "127.0.0.1", port, scenario, "--quiet");
      }
      clientPIDs.push_back(clientPID);
    }

    for (pid_t pid : clientPIDs) {
      logger.logPID(pid);
    }

    for (int i = 0; i < nbClients; ++i) {
      _wait();
    }
    logger.remove(nbClients);
  }
  std::cout << "\t\tAll scenarios with " << name << " finished\n";
}

int main(int argc, char **argv) {
  const char *cert = CERT_DEFAULT, *key = KEY_DEFAULT;
  int min = MIN_DEFAULT, max = MAX_DEFAULT, step = STEP_DEFAULT,
      repeat = REPEAT_DEFAULT;

  parseArgs(argc, argv, cert, key, min, max, step, repeat);

  Logger logger(LOGGER_IN, LOGGER_OUT);

  auto serverPIDs = startServers(cert, key);
  for (pid_t pid : serverPIDs) {
    logger.logPID(pid);
  }
  for (int i = 0; i < repeat; ++i) {
    std::cout << "Run #" << i + 1 << "\n";

    for (int j = min; j <= max; j += step) {
      std::cout << "\tStarting scenarios with " << j << " clients\n";

      runClients(j, logger, "picoquic", "build/picoquic_impl/pico_client",
                 PICO_PORT);

      runClients(j, logger, "msquic", "build/msquic_impl/ms_client", MS_PORT);

      runClients(j, logger, "mvfst", "build/quic/mvclient", MV_PORT);

      runClients(j, logger, "tcp", "build/tcp/tcp_client", TCP_PORT);
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