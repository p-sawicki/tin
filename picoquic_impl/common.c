#include "common.h"
#include <picosocks.h>
#include <stdio.h>
#include <string.h>

const size_t packet_sizes[] = {100, 10485760, 1073741824};

int get_port(char const *sample_name, char const *port_arg) {
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

FILE *open_log() {
  char *pid = malloc(11);
  sprintf(pid, "%d", getpid());

  char *log_file_path =
      malloc(strlen(PICOQUIC_CLIENT_QLOG_DIR) + strlen(pid) + 2);
  strcpy(log_file_path, PICOQUIC_CLIENT_QLOG_DIR);
  strcat(log_file_path, "/");
  strcat(log_file_path, pid);

  return fopen(log_file_path, "w");
}