#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "common.h"
#define MAX 80
#define PORT 8080
#define SA struct sockaddr

void func(int sockfd) {
  char buff[MAX] = "Czesc";
  printf("Send '%s' to server\n", buff);
  write(sockfd, buff, sizeof(buff));
  bzero(buff, sizeof(buff));
}

int create_connection(char const *server_name, int server_port) {
	int sockfd;
  struct sockaddr_in servaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    printf("Staruje klient na port %d i ip %s\n", server_port, server_name);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");

  servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(server_port);

	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
		printf("connection with the server failed...\n");
		exit(0);
	}
	return sockfd;
}

void show_certs(SSL* ssl)
{
  X509 *cert;
  char *line;
  cert = SSL_get_peer_certificate(ssl);
  if ( cert != NULL )
  {
      printf("Server certificates:\n");
      line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
      printf("Subject: %s\n", line);
      free(line);
      line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
      printf("Issuer: %s\n", line);
      free(line);
      X509_free(cert);
  }
  else
      printf("Info: No client certificates configured.\n");
}

int client_loop(SSL *ssl){
  char buf[1024];
  int bytes;

  SSL_write(ssl, "Case 3", strlen("Case 3"));

  while (bytes = SSL_read(ssl, buf, sizeof(buf))) {
    printf("Received: \"%s\"\n", buf);
    bzero(buf, sizeof(buf));
  }
}

int tcp_client(char const *server_name, int server_port) {
  printf("Starting tcp client on port %d\n", server_port);
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
    client_loop(ssl);
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

enum packet_size_t { PACKET_SMALL, PACKET_MED, PACKET_LARGE };

int main(int argc, char **argv) {

	int exit_code = 0;
	if (argc < 4) {
		usage(argv[0]);
	} else {

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

    exit_code = tcp_client(argv[1], server_port);
  }
}
