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

int create_server(char const *server_name, int server_port) {
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

int server_loop(int sockfd, SSL_CTX *ctx, SSL *ssl1) {
	int msgsock, rval;
	char buf[100];
	unsigned long ssl_err = 0;
	SSL *ssl;
	do {
        msgsock = accept(sockfd,(struct sockaddr *) 0,(int *) 0);
        if (msgsock == -1 )
             perror("accept");
        else do {
			ssl = SSL_new(ctx);
        	SSL_set_fd(ssl, msgsock);
			memset(buf, 0, sizeof buf);
			if (SSL_accept(ssl) <= 0) {
				ssl_err = SSL_get_error(ssl, -1);
				print_error_string(ssl_err, "Problem pomcy");
            	ERR_print_errors_fp(stderr);
        	} else {
				SSL_read(ssl, buf, sizeof(buf));
				printf("Server otrzymal: \"%s\"\n", buf);
				memset(buf, 0,  sizeof(buf));
				for(int i=0; i<1; ++i)
					SSL_write(ssl, "Invalid Message", strlen("Invalid Message"));
			}
        } while (0);
        SSL_free(ssl);
        close(msgsock);
    } while(1);
}

int tcp_server(char const *server_name, int server_port,
			   const char *pem_cert, const char *pem_key) {
  	printf("Starting tcp server on port %d\n", server_port);
	int sockfd;
	SSL_CTX *ctx;
	SSL *ssl;

    init_openssl();
    ctx = create_context(SERVER_CONTEXT);

    configure_context(ctx, pem_cert, pem_key);

	sockfd = create_server(server_name, server_port);

	server_loop(sockfd, ctx, ssl);

	close(sockfd);
    SSL_CTX_free(ctx);
}

void usage(char const *name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "    %s ip_addr port cert_file private_key_file\n", name);
  exit(1);
}

int main(int argc, char **argv) {
	int exit_code = 0;

	if (argc < 5) {
		usage(argv[0]);
	} else {
		int server_port = atoi(argv[2]);
		exit_code = tcp_server(argv[1], server_port, argv[3], argv[4]);
	}

  	exit(exit_code);
}
