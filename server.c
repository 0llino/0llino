#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAX 160
#define PORT 8080
#define SA struct sockaddr

int clientCount = 0, top = -1;
char stack[MAX][1024];
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

int push(char stack[MAX][1024], int *top, char data[1024])
{
      if(*top == MAX -1)
            return(-1);
      else {
            *top = *top + 1;
            strcpy(stack[*top], data);
            return(1);
      }
}

int pop(char stack[MAX][1024], int *top, char data[1024])
{
      if(*top == -1)
            return(-1);
      else {
            strcpy(data, stack[*top]);
            *top = *top - 1;
            return(1);
      }
}

// sleep() only works in seconds, usleep works in nano seconde (and then, msleep works in miliseconds)
int msleep(unsigned int tms) {
  return usleep(tms * 1000);
}

//Allows to catch a specific word of the data send by the user
char * parseWord( const char str[], size_t pos ){
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
	for(int i = 0; i < clientCount; i++){
		send(Client[i].sockID, dataOut, sizeof(buffer), 0);
	}
}

void * Dispatcher(){
	char data[1024];
	int isNull;
	while(1){
		isNull = pop(stack, &top, data);
		if(isNull != -1){
			pop(stack, &top, data);
			msleep(20);
			broadcastClient(data);
		}
	}
	return NULL;
}

// functon handeling connexions
void * clientListener(void * ClientDetail){
	// variables
	struct client* clientDetail = (struct client*) ClientDetail;
	int index = clientDetail -> index;
	int clientSocket = clientDetail -> sockID;
	Client[index].pseudo = "default pseudo";
	clientDetail -> grpID = "all";
	char pseudo[254], dataIn[1024], dataOut[1024];
	
	//pipe through fork
	int fd[2];
	pipe(fd);

	//Waits for the pseudonym of the client
	int receved = recv(clientSocket, pseudo, 254, 0);

	Client[index].pseudo = pseudo;
	strcpy(dataOut, Client[index].pseudo);
	strcat(dataOut, " is connected !\n");
	push(stack, &top, dataOut);
	//broadcastClient(dataOut);

	pid_t pid = fork();
	if(pid == -1)
		return NULL;

	else if(pid == 0){
		while(1){
			bzero(dataIn, 1024);
			receved = recv(clientSocket, dataIn, 1024, 0);
			dataIn[receved] = '\0';
			write(fd[1], dataIn, sizeof(dataIn));
		}
	} else { 

		while(1){
			bzero(dataIn, 1024);
			bzero(dataOut, 1024);
			//int read = recv(clientSocket, dataIn, 1024, 0);
			read(fd[0], dataIn, sizeof(dataIn));

			char *firstWord = parseWord(dataIn, 0);
			char *secondWord = parseWord(dataIn, 1);
			char *thirdWord = parseWord(dataIn, 2);
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
				strcpy(dataOut, Client[index].pseudo);
				strcat(dataOut, " has disconnected...\n");
				broadcastClient(dataOut);
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


					strcpy(dataOut, "\033[0;31m");
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
				push(stack, &top, dataOut);
			}
		}
	}
	//pthread_exit(NULL); -> mettre pthread exit ne vire pas l'utilisateur quand il quitte (wtf)
	return NULL;
}

// main
int main()
{
	struct sockaddr_in servaddr;
	pthread_t thread_dispatcher;

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

	pthread_create(&thread_dispatcher, NULL, Dispatcher, NULL);

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