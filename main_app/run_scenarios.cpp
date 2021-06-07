// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 15.05.2021
// Autor: Piotr Sawicki

#include "Logger.hpp"
#include "utils.hpp"

const char *CERT_DEFAULT = "cert.pem", *KEY_DEFAULT = "key.pem",
           *LOGGER_IN = "/logger_in", *LOGGER_OUT = "/logger_out",
           *PICO_PORT = "4433", *MS_PORT = "4434", *TCP_PORT = "4435";
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

pid_t startServer(const char *path, const char *port, const char *cert,
                  const char *key, const Logger &logger) {
  pid_t pid = _fork();
  if (pid == 0) {
    _execl(path, port, cert, key, "--quiet");
  }

  logger.logPID(pid);
  return pid;
}

void killServer(pid_t pid, const Logger &logger) {
  logger.remove(1);

  kill(pid, SIGKILL);
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

void runImpl(const char *serverPath, const char *clientPath, const char *port,
             const char *cert, const char *key, const Logger &logger,
             int nbClients, const char *name) {
  pid_t pid = startServer(serverPath, port, cert, key, logger);
  runClients(nbClients, logger, name, clientPath, port);
  killServer(pid, logger);
}

int main(int argc, char **argv) {
  const char *cert = CERT_DEFAULT, *key = KEY_DEFAULT;
  int min = MIN_DEFAULT, max = MAX_DEFAULT, step = STEP_DEFAULT,
      repeat = REPEAT_DEFAULT;

  parseArgs(argc, argv, cert, key, min, max, step, repeat);

  Logger logger(LOGGER_IN, LOGGER_OUT);

  for (int i = 0; i < repeat; ++i) {
    std::cout << "Run #" << i + 1 << "\n";

    for (int j = min; j <= max; j += step) {
      std::cout << "\tStarting scenarios with " << j << " clients\n";

      runImpl("build/picoquic_impl/pico_server",
              "build/picoquic_impl/pico_client", PICO_PORT, cert, key, logger,
              j, "picoquic");

      runImpl("build/msquic_impl/ms_server", "build/msquic_impl/ms_client",
              MS_PORT, cert, key, logger, j, "msquic");

      runImpl("build/tcp/tcp_server", "build/tcp/tcp_client", TCP_PORT, cert,
              key, logger, j, "tcp");
    }
  }
  std::cout << "All runs finished. Goodbye\n";
}