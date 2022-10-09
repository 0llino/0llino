#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

void recvMessage(int sock)
{
	char buff[MAX];
	for (;;) {
        
        // like 'xor buff, buff'
		bzero(buff, MAX);
        read(sock, buff, sizeof(buff));
		printf("From Server : %s", buff);
		
        if ((strncmp(buff, "exit", 4)) == 0) {
			printf("Client Exit...\n");
			break;
		}
	}
}

void sendMessage(int sock)
{
	char buff[MAX];
	int n;
	for (;;) { 
        // like 'xor buff, buff'
		bzero(buff, MAX);
		printf("-> ");
		n = 0;
		
        while ((buff[n++] = getchar()) != '\n');
		write(sock, buff, sizeof(buff));
		
        if ((strncmp(buff, "exit", 4)) == 0) {
			printf("Client Exit...\n");
			break;
		}
	}
}

int main()
{
	int sock;
	struct sockaddr_in servaddr;

	// socket create and verification
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		perror("socket creation error\n");
		exit(0);
	}
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(PORT);

	// connect the client socket to server socket
	if (connect(sock, (SA*)&servaddr, sizeof(servaddr)) != 0) {
		perror("connection with server error");
		exit(0);
	}
	else printf("connected to the server..\n");

	pid_t pid = fork();
	if (pid == -1){
		perror("fork error");
		return EXIT_FAILURE;
	} else if (pid == 0) {
		sendMessage(sock);
	} else {
		recvMessage(sock);
	}

	// close the socket
	close(sock);
}
