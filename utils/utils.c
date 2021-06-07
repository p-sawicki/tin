// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 29.05.2021
// Autor: Piotr Sawicki

#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const size_t packet_sizes[] = {100, 10485760, 1073741824};

int get_port(char const *port_arg) {
  int server_port = atoi(port_arg);
  if (server_port <= 0) {
    fprintf(stderr, "Invalid port: %s\n", port_arg);
  }

  return server_port;
}

int is_valid_packet_size(enum packet_size_t size) {
  if (size >= PACKET_SMALL && size <= PACKET_LARGE) {
    return 1;
  }
  return 0;
}

FILE *get_log(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--quiet")) {
      return fopen("/dev/null", "w");
    }
    if (!strcmp(argv[i], "--log")) {
      char pid[11];
      sprintf(pid, "%d", getpid());

      char log_file_path[strlen("logs") + strlen(pid) + 2];
      strcpy(log_file_path, "logs");
      strcat(log_file_path, "/");
      strcat(log_file_path, pid);

      return fopen(log_file_path, "w");
    }
  }

  return stdout;
}

int delay(int argc, char **argv, int nb_packets) {
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--no-delay")) {
      return 0;
    }
  }

  srand(getpid());
  sleep(rand() % (60 / nb_packets));
  return 1;
}

void get_packet_info(int scenario, int *nb_packets,
                     enum packet_size_t *packet_size) {
  switch (scenario) {
  case 1:
    *packet_size = PACKET_SMALL;
    *nb_packets = 1;
    break;
  case 2:
    *packet_size = PACKET_LARGE;
    *nb_packets = 1;
    break;
  case 3:
    *packet_size = PACKET_SMALL;
    *nb_packets = 10;
    break;
  case 4:
    *packet_size = PACKET_MED;
    *nb_packets = 10;
    break;
  default:
    printf("Unsupported scenario!\n");
    exit(EXIT_FAILURE);
  }
}