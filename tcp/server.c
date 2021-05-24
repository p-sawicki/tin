#include "common.h"
#define MAX 80
#define PORT 8080
#define SA struct sockaddr

void configure_context(SSL_CTX *ctx, const char* pem_cert, const char* pem_key)
{
    SSL_CTX_set_ecdh_auto(ctx, 1);

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, pem_cert, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, pem_key, SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
    }
}

// Function designed for chat between client and server.
void func(int sockfd)
{
	char buff[MAX];
	int n;
	bzero(buff, MAX);

	read(sockfd, buff, sizeof(buff));
	printf("From client: %s\t");
	bzero(buff, MAX);
	n = 0;
	// write(sockfd, buff, sizeof(buff));
}

int create_server(int server_port) {
	int sockfd, len;
	struct sockaddr_in servaddr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(server_port);

	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
		printf("socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");

    len = sizeof( servaddr);
    if (getsockname(sockfd,(struct sockaddr *) &servaddr, &len)
         == -1) {
        perror("getting socket name");
        exit(1);
    }
    printf("Socket port #%d\n", ntohs(servaddr.sin_port));
	// Now server is ready to listen and verification
	if ((listen(sockfd, 5)) != 0) {
		printf("Listen failed...\n");
		exit(0);
	}
	else
		printf("Server listening..\n");
	return sockfd;
}

void print_error_string(unsigned long err, const char* const label)
{
    const char* const str = ERR_reason_error_string(err);
    if(str)
        fprintf(stderr, "%s\n", str);
    else
        fprintf(stderr, "%s failed: %lu (0x%lx)\n", label, err, err);
}
#include <stdint.h>

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
             perror("accept");
        else do {
			ssl = SSL_new(ctx);
        	SSL_set_fd(ssl, msgsock);
			if (SSL_accept(ssl) <= 0) {
				ssl_err = SSL_get_error(ssl, -1);
				print_error_string(ssl_err, "Problem");
            	ERR_print_errors_fp(stderr);
        	} else {
				temp='1';
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
  	printf("Starting tcp server on port %d\n", server_port);
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

int main(int argc, char **argv) {
	int exit_code = 0;

	if (argc < 4) {
		usage(argv[0]);
	} else {
		int server_port = atoi(argv[1]);
		exit_code = tcp_server(server_port, argv[2], argv[3]);
	}

  	exit(exit_code);
}
