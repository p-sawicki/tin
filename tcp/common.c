// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 29.05.2021
// Autor: Adam Solawa

#include "common.h"

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
