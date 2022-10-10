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

// functon handeling connexions
void * doNetworking(void * ClientDetail){
	// 
	struct client* clientDetail = (struct client*) ClientDetail;
	int index = clientDetail -> index;
	int clientSocket = clientDetail -> sockID;
	char dataIn[1024];
	char dataOut[1024];
	printf("Client %d connected, with a socketID of %d\n", index + 1, clientSocket);

	pid_t pid = fork();

	if(pid == -1){
		perror("fork error");
	} else if (pid == 0) {
		// Receive messages
		while(1){
			bzero(dataIn, 1024);
			int read = recv(clientSocket, dataIn, 1024, 0);
			dataIn[read] = '\0';
			printf("%s", dataIn);
		}
	} else {
		// Send messages
		int n = 0;
		while(1){
			bzero(dataOut, 1024);
			n = 0;
			while ((dataOut[n++] = getchar()) != '\n');
			for (int i = 0; i < clientCount; i++){
				write(Client[clientCount].sockID, dataOut, sizeof(dataOut));
			}
			//write(clientSocket, dataOut, sizeof(dataOut));
		}
	}

	return NULL;
}

// main
int main()
{
	struct sockaddr_in servaddr;

	// socket create & verification
	int serverSocket = socket(PF_INET, SOCK_STREAM, 0);

	if (serverSocket == -1) {
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
	close(serverSocket);
}