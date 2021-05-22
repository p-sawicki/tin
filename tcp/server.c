#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#define MAX 80
#define PORT 8080
#define SA struct sockaddr

void init_openssl()
{ 
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
    EVP_cleanup();
}

SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
	perror("Unable to create SSL context");
	ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }

    return ctx;
}

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

int server_loop(int sockfd) {
	int msgsock, rval;
	char buf[100];
	do {
        msgsock = accept(sockfd,(struct sockaddr *) 0,(int *) 0);
        if (msgsock == -1 )
             perror("accept");
        else do {
             memset(buf, 0, sizeof buf);
             if ((rval = read(msgsock,buf, 80)) == -1)
                 perror("reading stream message");
             if (rval == 0)
                 printf("Ending connection\n");
             else
                 printf("-->%s\n", buf);
        } while (rval != 0);
        close(msgsock);
    } while(1);
}

int tcp_server(char const *server_name, int server_port,
			   const char *pem_cert, const char *pem_key) {
  	printf("Starting tcp server on port %d\n", server_port);
	int sockfd;
	SSL_CTX *ctx;

    init_openssl();
    ctx = create_context();

    configure_context(ctx, pem_cert, pem_key);

	sockfd = create_server(server_name, server_port);

	server_loop(sockfd);

	close(sockfd);
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
