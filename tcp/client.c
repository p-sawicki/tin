#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "common.h"

FILE *log_file;

int create_connection(char const *server_name, int server_port) {
	int sockfd;
  struct sockaddr_in servaddr;
  struct hostent *hp;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
  fprintf(log_file, "Staruje klient na port %d i ip %s\n", server_port, server_name);
	if (sockfd == -1) {
		fprintf(log_file, "socket creation failed...\n");
		exit(0);
	}
	else
		fprintf(log_file, "Socket successfully created..\n");

  hp = gethostbyname(server_name);
  if (hp == (struct hostent *) 0) {
    fprintf(log_file, "%s: unknown host\n", server_name);
    exit(2);
  }
  memcpy((char *) &servaddr.sin_addr, (char *) hp->h_addr, hp->h_length);

  servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(server_port);

	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
		fprintf(log_file, "connection with the server failed...\n");
		exit(0);
	}
	return sockfd;
}

void show_certs(SSL* ssl)
{
  X509 *cert;
  char *line;
  cert = SSL_get_peer_certificate(ssl);
  if ( cert != NULL ) {
    fprintf(log_file, "Server certificates:\n");
    line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
    fprintf(log_file, "Subject: %s\n", line);
    free(line);
    line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
    fprintf(log_file, "Issuer: %s\n", line);
    free(line);
    X509_free(cert);
  }
  else
    fprintf(log_file, "Info: No client certificates configured.\n");
}

int client_loop(SSL *ssl, int nb_packets, enum packet_size_t packet_size){
  uint8_t byte;
  int bytes;
  char buffer[SEGMENT_SIZE];
  sprintf(buffer, "%d,%d", (int) packet_size, nb_packets);

  SSL_write(ssl, buffer, strlen(buffer));

  while (bytes = SSL_read(ssl, buffer, sizeof(buffer))) {
    fprintf(log_file, "Received: %s\n", buffer);
  }
}

int tcp_client(char const *server_name, int server_port,
               int nb_packets, enum packet_size_t packet_size) {
  fprintf(log_file, "Starting tcp client on port %d\n", server_port);
  SSL_CTX *ctx;
  SSL *ssl;
	int sockfd;

  init_openssl();
  ctx = create_context(CLIENT_CONTEXT);

	sockfd = create_connection(server_name, server_port);
  ssl = SSL_new(ctx);
  SSL_set_fd(ssl, sockfd);
  if ( SSL_connect(ssl) == -1 )
      ERR_print_errors_fp(stderr);
  else {
    show_certs(ssl);
    client_loop(ssl, nb_packets, packet_size);
    SSL_free(ssl);
  }

  SSL_CTX_free(ctx);
	close(sockfd);
}


void usage(char const *name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "    %s server_name port scenario\n", name);
  exit(1);
}

int main(int argc, char **argv) {

	int exit_code = 0;
	if (argc < 4) {
		usage(argv[0]);
	} else {
    log_file = open_log();

    int server_port = atoi(argv[2]);
    int scenario = atoi(argv[3]);
    int nb_packets;
    enum packet_size_t packet_size;

    switch (scenario) {
    case 1:
      packet_size = PACKET_SMALL;
      nb_packets = 1;
      break;
    case 2:
      packet_size = PACKET_LARGE;
      nb_packets = 1;
      break;
    case 3:
      packet_size = PACKET_SMALL;
      nb_packets = 10;
      break;
    case 4:
      packet_size = PACKET_MED;
      nb_packets = 10;
      break;
    }

    exit_code = tcp_client(argv[1], server_port, nb_packets, packet_size);
    fclose(log_file);
  }
}
