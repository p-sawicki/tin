#include "common.h"

const size_t packet_sizes[] = {100, 10485760, 1073741824};

SSL_CTX *create_context(enum contex_type type) {
  const SSL_METHOD *method;
  SSL_CTX *ctx;

  switch (type) {
  case SERVER_CONTEXT:
    method = TLS_server_method();
    break;
  case CLIENT_CONTEXT:
    method = TLS_client_method();
    break;
  default:
    perror("Wrong SSL contex type");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
    break;
  }

  ctx = SSL_CTX_new(method);
  if (!ctx) {
    perror("Unable to create SSL context");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  return ctx;
}

void init_openssl() {
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() { EVP_cleanup(); }

FILE *open_log() {
  char *pid = malloc(11);
  sprintf(pid, "%d", getpid());

  char *log_file_path = malloc(strlen(TCP_CLIENT_LOG_DIR) + strlen(pid) + 2);
  strcpy(log_file_path, TCP_CLIENT_LOG_DIR);
  strcat(log_file_path, "/");
  strcat(log_file_path, pid);

  return fopen(log_file_path, "w");
}
