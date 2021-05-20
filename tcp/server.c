#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#define MAX 80
#define PORT 8080
#define SA struct sockaddr

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
             if ((rval = read(msgsock,buf, 1024)) == -1)
                 perror("reading stream message");
             if (rval == 0)
                 printf("Ending connection\n");
             else
                 printf("-->%s\n", buf);
        } while (rval != 0);
        close(msgsock);
    } while(1);
}

int tcp_server(char const *server_name, int server_port) {
  	printf("Starting tcp server on port %d\n", server_port);
	int sockfd;

	sockfd = create_server(server_name, server_port);

	server_loop(sockfd);

	close(sockfd);
}

void usage(char const *name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "    %s ip_addr port\n", name);
  exit(1);
}

int main(int argc, char **argv) {
	int exit_code = 0;

	if (argc < 2) {
		usage(argv[0]);
	} else {
		int server_port = atoi(argv[2]);
		exit_code = tcp_server(argv[1], server_port);
	}

  	exit(exit_code);
}
