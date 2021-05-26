#include <stdint.h>
#include <signal.h>
#include "common.h"
#define SA struct sockaddr

FILE *log_file;

void configure_context(SSL_CTX *ctx, const char* pem_cert, const char* pem_key)
{
  SSL_CTX_set_ecdh_auto(ctx, 1);

  if (SSL_CTX_use_certificate_file(ctx, pem_cert, SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(log_file);
    exit(EXIT_FAILURE);
  }

  if (SSL_CTX_use_PrivateKey_file(ctx, pem_key, SSL_FILETYPE_PEM) <= 0 ) {
    ERR_print_errors_fp(log_file);
    exit(EXIT_FAILURE);
  }
}

int create_server(int server_port) {
	int sockfd, len;
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

	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
		fprintf(log_file, "socket bind failed...\n");
		exit(0);
	}
	else
		fprintf(log_file, "Socket successfully binded..\n");

  len = sizeof( servaddr);
  if (getsockname(sockfd,(struct sockaddr *) &servaddr, &len) == -1) {
    fprintf(log_file, "Getting socket name\n");
    exit(1);
  }
  fprintf(log_file, "Socket port #%d\n", ntohs(servaddr.sin_port));
	if ((listen(sockfd, 5)) != 0) {
		fprintf(log_file, "Listen failed...\n");
		exit(0);
	}
	else
		fprintf(log_file, "Server listening..\n");
	return sockfd;
}

void print_error_string(unsigned long err, const char* const label)
{
  const char* const str = ERR_reason_error_string(err);
  if(str)
    fprintf(log_file, "%s\n", str);
  else
    fprintf(log_file, "%s failed: %lu (0x%lx)\n", label, err, err);
}

int server_loop(int sockfd, SSL_CTX *ctx, SSL *ssl1) {
	int msgsock, rval;
	uint8_t byte;
	int nb_packet;
	unsigned long ssl_err = 0;
	uint32_t tmp,n;
	int packet_size;
	char buffer[SEGMENT_SIZE];
	char *temp;
	SSL *ssl;
  
	do {
    msgsock = accept(sockfd,(struct sockaddr *) 0,(int *) 0);
    if (msgsock == -1 )
      fprintf(log_file, "connection acceptin failed");
    else do {
			ssl = SSL_new(ctx);
      SSL_set_fd(ssl, msgsock);

			if (SSL_accept(ssl) <= 0) {
				ssl_err = SSL_get_error(ssl, -1);
				print_error_string(ssl_err, "Problem");
        ERR_print_errors_fp(log_file);
      } else {
				SSL_read(ssl, buffer, sizeof(char)*3);
				temp = strtok(buffer, ",");
				packet_size = atoi(temp);
				temp = strtok(NULL, ",");
				nb_packet = atoi(temp);
				for(int i=0; i<nb_packet; ++i){
					for(int j=0; SEGMENT_SIZE * j < packet_sizes[packet_size] ; ++j ) {
						sprintf(buffer, "nb: %d", j * SEGMENT_SIZE);
						SSL_write(ssl, buffer, sizeof(buffer));
					}
				}
			}
    } while (0);
    SSL_free(ssl);
    close(msgsock);
  } while(1);
}

int tcp_server(int server_port,
			   const char *pem_cert, const char *pem_key) {
  fprintf(log_file, "Starting tcp server on port %d\n", server_port);
	int sockfd;
	SSL_CTX *ctx;
	SSL *ssl;

  init_openssl();
  ctx = create_context(SERVER_CONTEXT);
  configure_context(ctx, pem_cert, pem_key);

	sockfd = create_server(server_port);

	server_loop(sockfd, ctx, ssl);

	close(sockfd);
  SSL_CTX_free(ctx);
}

void usage(char const *name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "    %s port cert_file private_key_file\n", name);
  exit(1);
}

void cleanup(int a)
{
  fclose(log_file);
  exit(0);
}

int main(int argc, char **argv) {
	int exit_code = 0;

	if (argc < 4) {
		usage(argv[0]);
	} else {
		log_file = open_log();
    signal(SIGINT, cleanup);

		int server_port = atoi(argv[1]);
		exit_code = tcp_server(server_port, argv[2], argv[3]);
	}

  exit(exit_code);
}
