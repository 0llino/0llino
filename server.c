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
static pthread_mutex_t mutex; 
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

struct client {
	int index;
	int sockID;
	int grpID;
	char *pseudo;
	struct sockaddr_in clientAddr;
	int len;
};

struct client Client[1024];
pthread_t thread[1024]; 

void broadcastClient(char *dataOut){
	char buffer[1024];
	for(int i = 0; i < clientCount; i++){
		send(Client[i].sockID, dataOut, sizeof(buffer), 0);
	}
}

// functon handeling connexions
void * clientListener(void * ClientDetail){
	// var
	struct client* clientDetail = (struct client*) ClientDetail;
	int index = clientDetail -> index;
	int clientSocket = clientDetail -> sockID;
	char pseudo[254];
	char dataIn[1024];
	char dataOut[1024];
	//Waits for the pseudonym of the client
	int read = recv(clientSocket, pseudo, 254, 0);
	Client[index].pseudo = pseudo;

	strcpy(dataOut, Client[index].pseudo);
	strcat(dataOut, " is connected !\n");
	broadcastClient(dataOut);
	/*for(int i = 0; i < clientCount; i++){
		send(Client[i].sockID, dataOut, sizeof(dataOut), 0);
	}*/

	while(1){
		bzero(dataIn, 1024);
		bzero(dataOut, 1024);
		int read = recv(clientSocket, dataIn, 1024, 0);
		dataIn[read] = '\0';
		printf("%s : %s", Client[index].pseudo, dataIn);
			
		
		// Allows to list all online clients, and sends it to the client
		if ((strncmp(dataIn, "/list", 5)) == 0) {
			strcpy(dataOut, "\033[0;32m");
			for (int i = 0; i < clientCount; i++) {
				if ((strncmp(Client[i+1].pseudo, "\0", 1)) != 0){
					strcat(dataOut, Client[i+1].pseudo);
					dataOut[strlen(dataOut)] = '\n';
				}
			}
			dataOut[strlen(dataOut)] = '\n';
			strcat(dataOut, "\033[0m");
			send(clientSocket, dataOut, strlen(dataOut), 0);
		} 

		// Allows client to exit
		else if ((strncmp(dataIn, "/exit", 5)) == 0) {
			send(clientSocket, "/exit", sizeof("/exit"), 0);
			close(clientSocket);
			clientDetail -> sockID = 0;
			break;
		}

		// Send message to all clients
		else{
			strcpy(dataOut, "\033[0;31m");
			strcat(dataOut, Client[index].pseudo);
			strcat(dataOut, " : ");
			strcat(dataOut, dataIn);
			strcat(dataOut, "\033[0m");
			broadcastClient(dataOut);
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
		pthread_mutex_init(&mutex, NULL);
		newClient = Client[clientCount].len;
		Client[clientCount].sockID = accept(serverSocket, (struct sockaddr*) &Client[clientCount].clientAddr, &newClient);
		Client[clientCount].index = clientCount + 1;
		
		pthread_create(&thread[clientCount], NULL, clientListener, (void *) &Client[clientCount]);
		
		clientCount ++;
		pthread_mutex_destroy(&mutex);
	}

	//for(int i = 0 ; i < clientCount ; i ++)
	//	pthread_join(thread[i], NULL);
	// After chatting close the socket
	close(serverSocket);
}