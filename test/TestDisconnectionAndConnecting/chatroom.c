#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include  <signal.h>
#include <semaphore.h> 
#include <error.h>

#define MAXCLIENTS 8
#define MAX 20
#define MAXMSG 500000
#define PORT 32000

/*this is structure of client*/
typedef struct {
	int index; /*position of client array */
	int sd; /*file descriptor of opened socket*/
	pthread_t tid; /*id of client thread*/
	struct sockaddr_in cliaddr;
	socklen_t clilen;
} client_t;

typedef struct {
	char *message;
}broadcasrT;

sem_t items; 
sem_t lock; 
sem_t signalB; 
sem_t space; 
sem_t enter;
char* queue[MAX];/*created queue for getting messages from clients and store in this queue and broadcast from this until empty queue. Here customer producer classical method is used to synchronize */

int front=-1,rear=-1;/*these are start and end position of queue*/
void enqueue(char* message);/*insert string to queue*/
char* dequeue(void);/*get string from queue*/
void display_queue(void);
int empty(void);/*check weather queue is empty or not*/
void* broadcast_msg(void* args);/*send same message for all of client at same time*/
static client_t *clients[MAXCLIENTS];/*new clients are stored in this array */
static volatile sig_atomic_t quit = 0;/*to handle the signal (ctrl+c)*/
static int currectClient=0;
void* handle_client(void* args);/*get message from client insert that message to queue*/
void cleanup(int signal);/*to control signal for ctrl+c*/
int next_free(void);/*find next free index of array of client*/
static int broad;

client_t* getClientDetails(int index,int sd,pthread_t tid,struct sockaddr_in cliaddr,socklen_t clilen);/*get new client information */
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER; 
int main(void)
{	
	int listenfd;
	int connfd;
	client_t* head;/*to create new client and store in this structure */
	struct sockaddr_in servaddr,cliaddr;
	socklen_t clilen;
	int next;/*next free index to be stored a client*/
	
	broadcasrT* mes;
	pthread_t brothread; /* broadcast thread to be start  */
	broad=0;
	
	listenfd=0;
	
	connfd=0;
	/*initialize semaphores */
	if(sem_init(&items,0,1)) 
	{ 
		printf("Could not initialize a semaphore\n"); 
		return -1; 
	}
	if(sem_init(&space,0,5)) 
	{ 
		printf("Could not initialize a semaphore\n"); 
		return -1; 
	}
	if(sem_init(&lock,0,1)) 
	{ 
		printf("Could not initialize a semaphore\n"); 
		return -1; 
	}
	if(sem_init(&signalB,0,0)) 
	{ 
		printf("Could not initialize a semaphore\n"); 
		return -1; 
	}
	if(sem_init(&enter,0,1)) 
	{ 
		printf("Could not initialize a semaphore\n"); 
		return -1; 
	}
	
	
	
	/*Use tcp*/
	listenfd=socket(AF_INET,SOCK_STREAM,0);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);

	/*listen to port 32000*/
	servaddr.sin_port=htons(PORT);

	bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

	listen(listenfd,MAXCLIENTS);
	
	clilen = sizeof(cliaddr);
	
	
	signal(SIGINT,cleanup);
	
	while (!quit)
	{
		
		pthread_t netthread;
		connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
		
		/*this semerphore is used to lock and unlock main thread */
		sem_wait(&lock);
		
		/*get next free index*/
		next=next_free();
		
		
		
		/*if next index is less than “currentThread” wont increate currentThread  otherwise increase currentThread  by one.*/
		if(next<currectClient){
			head=getClientDetails(next,connfd,netthread,cliaddr,clilen);
			clients[next]=head;
			
		}else{
			head=getClientDetails(currectClient,connfd,netthread,cliaddr,clilen);
			clients[currectClient]=head;
			currectClient++;
			
		}
		
		/*unlock main thread*/
		sem_post(&lock);
		/* start broadcast thread and new thread for each connection.Here broadcast thread is initialize only one time that is done by “broad” variable */
		if (broad==0)
		{	
			/**/
			mes=malloc(sizeof(broadcasrT));
			mes->message=malloc(sizeof(char)*100);
			strcpy(mes->message,"Broadcast thread has been started\n");
			if( pthread_create( &brothread, NULL,broadcast_msg,(void*)mes)){
			printf("error creating thread.");
			abort();}
			
			broad++;
		}else{
			
			
		}
		if ( pthread_create( &netthread, NULL,handle_client,(void*)head) )
		{
			printf("error creating thread.");
			abort();
		}
		
		
		
		
	}
	
	close(listenfd);
	free(clients[MAXCLIENTS]);
	free(queue[MAX]);
	
return 0;
}

void* broadcast_msg(void* args)
{	
	int jp;
	broadcasrT* mes;
	mes=(broadcasrT*)args;
	printf("%s\n",mes->message);
	while(!quit){
	/*send message to client*/
  	/*at the beginning there is no message to send that is what semaphore “signalB” does.It wait until somebody sends messages to server. */
	sem_wait(&signalB);
	/*this semaphore is lock bellow section to avoid the race conditions. */
	sem_wait(&items);
	
	
	jp=0;
	
	
	while(empty()){/*dequeu the message and sent it to each client c=which is connected to server write now.*/
		char mes[MAXMSG];
		strcpy(mes,dequeue());
		for(jp=0;jp<currectClient;jp++){
			
			
				sendto(clients[jp]->sd,mes,strlen(mes),0,(struct sockaddr *)&(clients[jp]->cliaddr),sizeof((clients[jp]->cliaddr)));
			
		

		}
	}
	sem_post(&items);
	
	sem_post(&space);
	}
	return NULL;
	
}
void* handle_client(void* args) 
{ 	
	int quit;
	client_t* newClient;
	int n;
	
	char *buffer1;
	char *buffer;
	char *newLineMessage;
	
	
	buffer=malloc(sizeof(char)*MAXMSG);/*store message coming from client*/
	
	
	
	quit=0;
	
	
	n=0;
	while(!quit){
	
	newClient=(client_t*)args;
	
	/*received data from client*/
	
	
	
	
	/*if message is bigger than the MAXMSG have to read more than ones because tcp it store and only sent when it read again.*/
	n=recvfrom(newClient->sd,buffer,MAXMSG,0,(struct sockaddr *)&(newClient->cliaddr),&(newClient->clilen));
	
	if (n == 0||n==-1) {
		/*n==0 connection is closed or n==-1 something wrong with client connection close the connection and put index -1 in client array I wont free that passion in array.I suppose it just wastage  of time.*/
		clients[newClient->index]->index=-1;
		close(newClient->sd);
		
		quit=1;
		
    		printf("Connection closed.\n");
    		
		
  	}
  	else{
		buffer[strlen(buffer)]='\0';
		newLineMessage=malloc(sizeof(char)*1000);/*message which is read line by line if it is big message and send line by line*/
		newLineMessage=strtok(buffer,"\n");
		sem_wait(&space);
		while (newLineMessage != NULL){
				/*read by line by  line and store in queue atthe same time put some headers about who send this message */
				buffer1=malloc(sizeof(char)*1000);/*message to be store in queue */
				strcpy(buffer1,newLineMessage);
				newLineMessage[0]='\0';
				strcat(buffer1,"\n");
				buffer1[strlen(buffer1)]='\0';
				
				
				/*put lock here to avoid race condition because If same time two thread(client) are accessed queue is going to be a big problem. */
				sem_wait(&enter);
				enqueue(buffer1);
				sem_post(&enter);
				newLineMessage= strtok(NULL,"\n");
				
  		}

		
		
		sem_post(&signalB);
	
	}
	}

	return NULL;	 
}

int next_free(void)
{	/*this is how find next free slot. currectClient is the one is used to store ccheck how many client is connected to server*/
	int j;
	
	for(j=0;j<currectClient;j++){
		
		if(clients[j]->index==-1){
			
			return j;
			
		}
	}
	
	
	return j+1;
}
client_t* getClientDetails(int index,int sd,pthread_t tid,struct sockaddr_in cliaddr,socklen_t clilen){
	client_t* newClient=malloc(sizeof(client_t));
	newClient->index=index;/*index of client if this is -1 client is disconnected*/
	newClient->sd=sd;/*file descriptor of newly created socket*/
	newClient->tid=tid;/*id of client it means thread id*/
	newClient->clilen=clilen;/*clent informations*/
	newClient->cliaddr=cliaddr;
	return newClient;
}


void cleanup(int signal){
	/*to  handle ctrl+c signal*/
	
	quit = 1;
	
	puts("Shutting down client connections...\n");
	sleep(1);
	exit(signal);
}
void enqueue(char* message)
{
  if(front==0 && rear==MAX-1){/*queueu is full*/
   
  }else if(front==-1&&rear==-1)
  {/*insert string into queue*/
      front=rear=0;
      queue[rear]=malloc(sizeof(char)*MAXMSG);
      strcpy( queue[rear],message);
 
  }
  else if(rear==MAX-1 && front!=0)
  {
    rear=0;
    queue[rear]=malloc(sizeof(char)*MAXMSG);
    strcpy( queue[rear],message);
  }
  else
  {
      rear++;
      queue[rear]=malloc(sizeof(char)*MAXMSG);
      strcpy( queue[rear],message);
  }
}
int empty(void){
	if(front==-1)
  	{
      /*check weather messages are there or not in queue*/	
	return 0;
  	}
	return 1;
}
char* dequeue(void)
{
  char* element;
  if(front==-1)
  {
    /*no elements in the queue*/
      return NULL;
  }else{
		/*return elements by elements*/
  element=malloc(sizeof(char)*MAXMSG);
  strcpy(element,queue[front]);
  if(front==rear)
     front=rear=-1;
  else
  {
    if(front==MAX-1)
      front=0;
    else
      front++;
     
  }
	
	return element;
 }
}
/*this is not needed to test queue is working properlly or not.*/
void display_queue(void)
{
    int i;
    if(front==-1){
     
   } else
    {
      
      for(i=front;i<=rear;i++)
      {
          printf("\n %s",queue[i]);
      }
    }
}
