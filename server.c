#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 8080
#define SA struct sockaddr

int clientCount = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

struct client {
	int index;
	int sockID;
	int grpID;
	struct sockaddr_in clientAddr;
	int len;
};

struct client Client[1024];
pthread_t thread[1024]; 

void * doNetworking(void * ClientDetail){

	struct client* clientDetail = (struct client*) ClientDetail;
	int index = clientDetail -> index;
	int clientSocket = clientDetail -> sockID;

	printf("Client %d connected, with a socketID of %d\n", index + 1, Client[index].sockID);

	while(1){

		char data[1024];
		int read = recv(clientSocket, data, 1024, 0);
		data[read] = '\0';
		printf("%s", data);

	}

	return NULL;
}

// Driver function
int main()
{
	int sockfd, connfd;
    unsigned int len;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	int serverSocket = socket(PF_INET, SOCK_STREAM, 0);

	if (sockfd == -1) {
		printf("socket creation failed...\n");
		return EXIT_FAILURE;
	}
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT, PROTOCOL
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	// Binding & listening verification
	if(bind(serverSocket,(struct sockaddr *) &servaddr , sizeof(servaddr)) == -1) {
		perror("error binding...");
		return EXIT_FAILURE;
	}
	if(listen(serverSocket, 1024) == -1){
		perror("error listening...");
		return EXIT_FAILURE;
	}
	else printf("Server listening...\n");

	// threads each new connection to the server, and links a struct to each client
	unsigned int newClient;
	while(1){
		newClient = Client[clientCount].len;
		Client[clientCount].sockID = accept(serverSocket, (struct sockaddr*) &Client[clientCount].clientAddr, &newClient);
		Client[clientCount].index = clientCount;

		pthread_create(&thread[clientCount], NULL, doNetworking, (void *) &Client[clientCount]);

		clientCount ++;
	}

	for(int i = 0 ; i < clientCount ; i ++)
		pthread_join(thread[i], NULL);
	// After chatting close the socket
	close(sockfd);
}
