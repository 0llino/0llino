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
	char *grpID;
	char *pseudo;
	struct sockaddr_in clientAddr;
	int len;
};

struct client Client[1024];
pthread_t thread[1024]; 

//Allows to catch a specific word of the data send by the user
char * GetToken( const char str[], size_t pos ){
    const char delim[] = " \t";
    char *inputCopy = malloc( ( strlen( str ) + 1 ) );
    char *p = NULL;

    if (inputCopy != NULL ){
        strcpy(inputCopy, str );
        p = strtok(inputCopy, delim );

        while ( p != NULL && pos -- != 0 ){
            p = strtok( NULL, delim );
        }
        if (p != NULL ){
            size_t n = strlen(p);
            memmove(inputCopy, p, n + 1);

            p = realloc(inputCopy, n + 1);
        }           
        if (p == NULL ){
            free(inputCopy );
        }
    }
    return p;
}

void broadcastClient(char *dataOut){
	char buffer[1024];
	pthread_mutex_lock(&mutex);
	for(int i = 0; i < clientCount; i++){
		send(Client[i].sockID, dataOut, sizeof(buffer), 0);
	}
	pthread_mutex_unlock(&mutex);
}

// functon handeling connexions
void * clientListener(void * ClientDetail){
	// var
	struct client* clientDetail = (struct client*) ClientDetail;
	int index = clientDetail -> index;
	int clientSocket = clientDetail -> sockID;
	clientDetail -> grpID = "all";
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

		char *firstWord = GetToken(dataIn, 0);
		char *secondWord = GetToken(dataIn, 1);
		char *thirdWord = GetToken(dataIn, 2);
		printf("%s : %s", Client[index].pseudo, dataIn);
			
		
		// Allows to list all online clients, and sends it to the client
		if ((strncmp(firstWord, "/list", 5)) == 0) {
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
		else if ((strncmp(firstWord, "/exit", 5)) == 0) {
			send(clientSocket, "/exit", sizeof("/exit"), 0);
			close(clientSocket);
			clientDetail -> sockID = 0;
			break;
		}

		// Privates messages
		else if ((strncmp(firstWord, "/msg", 4)) == 0) {
			int toRemove = strlen(secondWord) + 6; 
			// removes /msg bob at the beginning 
			memmove(dataIn, dataIn + toRemove, strlen(dataIn));

			// adds "bob to josh :" at the beginning 
			strcpy(dataOut, "\033[0;33m");
			strcat(dataOut, Client[index].pseudo);
			strcat(dataOut, " to ");
			strcat(dataOut, secondWord);
			strcat(dataOut, " : ");
			strcat(dataOut, dataIn);
			strcat(dataOut, "\033[0m");

			// if second word == pseudo ; then send message
			for(int i = 0; i < clientCount; i++){
				if ((strncmp(secondWord, Client[i+1].pseudo, strlen(Client[i+1].pseudo))) == 0){
					send(Client[i].sockID, dataOut, strlen(dataOut), 0);
				}
			}
		}

		else if ((strncmp(firstWord, "/grp", 4)) == 0) {
			// If you want to change of group
			if ((strncmp(secondWord, "set", 3)) == 0){
				thirdWord[strlen(thirdWord)-1] = '\0';
				clientDetail -> grpID = thirdWord;
			}
			// To send a message to all members of ur group
			else {
				memmove(dataIn, dataIn + 5, strlen(dataIn));


				strcpy(dataOut, "\033[0;33m");
				strcat(dataOut, Client[index].pseudo);
				strcat(dataOut, " to group ");
				strcat(dataOut, clientDetail -> grpID);
				strcat(dataOut, " : ");
				strcat(dataOut, dataIn);
				strcat(dataOut, "\033[0m");

				for(int i = 0; i < clientCount; i++){
					printf("%s : %s\n", Client[i+1].pseudo, Client[i].grpID);
					if (strncmp(clientDetail -> grpID, Client[i].grpID, strlen(clientDetail -> grpID)) == 0){
						send(Client[i].sockID, dataOut, strlen(dataOut), 0);
					}
				}
			}
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