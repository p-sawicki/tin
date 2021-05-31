#include "common.h"
#include "utils.h"
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#define SA struct sockaddr

FILE *log_file;
static char *buffers[3];

void configure_context(SSL_CTX *ctx, const char *pem_cert,
                       const char *pem_key) {
  SSL_CTX_set_ecdh_auto(ctx, 1);

  if (SSL_CTX_use_certificate_file(ctx, pem_cert, SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(log_file);
    exit(EXIT_FAILURE);
  }

  if (SSL_CTX_use_PrivateKey_file(ctx, pem_key, SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(log_file);
    exit(EXIT_FAILURE);
  }
}

int create_server(int server_port) {
  int sockfd;
  unsigned int len;
  struct sockaddr_in servaddr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    fprintf(log_file, "Socket creation failed...\n");
    exit(0);
  }

  fprintf(log_file, "Socket successfully created..\n");

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(server_port);

  if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0) {
    fprintf(log_file, "socket bind failed...\n");
    exit(0);
  } else
    fprintf(log_file, "Socket successfully binded..\n");

  len = sizeof(servaddr);
  if (getsockname(sockfd, (struct sockaddr *)&servaddr, (socklen_t *)&len) ==
      -1) {
    fprintf(log_file, "Getting socket name\n");
    exit(1);
  }
  fprintf(log_file, "Socket port #%d\n", ntohs(servaddr.sin_port));
  if ((listen(sockfd, SOMAXCONN)) != 0) {
    fprintf(log_file, "Listen failed...\n");
    exit(0);
  } else
    fprintf(log_file, "Server listening..\n");

  return sockfd;
}

void print_error_string(unsigned long err, const char *const label) {
  const char *const str = ERR_reason_error_string(err);
  if (str)
    fprintf(log_file, "%s\n", str);
  else
    fprintf(log_file, "%s failed: %lu (0x%lx)\n", label, err, err);
}

typedef struct {
  int msgsock;
  SSL_CTX *ctx;
} conn_params_t;

void *handle_connection(void *params) {
  int packet_size;
  unsigned long ssl_err = 0;
  SSL *ssl;
  conn_params_t *conn_params = (conn_params_t *)params;

  if (conn_params->msgsock == -1)
    fprintf(log_file, "connection acceptin failed");
  else {
    ssl = SSL_new(conn_params->ctx);
    SSL_set_fd(ssl, conn_params->msgsock);

    if (SSL_accept(ssl) <= 0) {
      ssl_err = SSL_get_error(ssl, -1);
      print_error_string(ssl_err, "Problem");
      ERR_print_errors_fp(log_file);
    } else {
      SSL_read(ssl, &packet_size, sizeof(packet_size));

      size_t to_write = packet_sizes[packet_size];
      char *buffer = buffers[packet_size];

      size_t written = 0;
      sprintf(buffer, "nb: %lu", to_write);
      for (; written < to_write;) {
        written += SSL_write(ssl, buffer + written, to_write - written);
      }
      SSL_free(ssl);
    }
  }

  close(conn_params->msgsock);
  free(conn_params);
  return 0;
}

int server_loop(int sockfd, SSL_CTX *ctx) {
  int msgsock, err;

  do {
    msgsock = accept(sockfd, (struct sockaddr *)0, (socklen_t *)0);

    conn_params_t *conn_params = (conn_params_t *)malloc(sizeof(conn_params_t));
    conn_params->ctx = ctx;
    conn_params->msgsock = msgsock;

    pthread_t handle;
    if ((err = pthread_create(&handle, NULL, handle_connection, conn_params)) !=
        0) {
      fprintf(log_file, "error creating thread: %s\n", strerror(err));
      return EXIT_FAILURE;
    } else {
      if ((err = pthread_detach(handle)) != 0) {
        fprintf(log_file, "error detaching thread: %s\n", strerror(err));
        return EXIT_FAILURE;
      }
    }

  } while (1);
}

int tcp_server(int server_port, const char *pem_cert, const char *pem_key) {
  fprintf(log_file, "Starting tcp server on port %d\n", server_port);
  int sockfd;
  SSL_CTX *ctx;

  init_openssl();
  ctx = create_context(SERVER_CONTEXT);
  configure_context(ctx, pem_cert, pem_key);

  sockfd = create_server(server_port);

  for (int i = 0; i < 3; ++i) {
    buffers[i] = (char *)malloc(packet_sizes[i]);
  }

  server_loop(sockfd, ctx);

  for (int i = 0; i < 3; ++i) {
    free(buffers[i]);
  }

  close(sockfd);
  SSL_CTX_free(ctx);
  return EXIT_FAILURE;
}

void usage(char const *name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "    %s port cert_file private_key_file\n", name);
  exit(1);
}

void cleanup(int a) {
  fclose(log_file);
  exit(0);
}

int main(int argc, char **argv) {
  int exit_code = 0;

  if (argc < 4) {
    usage(argv[0]);
  } else {
    log_file = get_log(argc, argv);
    signal(SIGINT, cleanup);

    int server_port = atoi(argv[1]);
    exit_code = tcp_server(server_port, argv[2], argv[3]);
  }

  exit(exit_code);
}
