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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define MAXMSG 5000
typedef struct {
	int sockfd;
	 char* buf;
	struct sockaddr_in servaddr;
	
} client_tSender;

typedef struct {
	 int sockfd;
	 char* buffer;
	 struct sockaddr_in servaddr;
	
} client_tReceiver;

void* handle_clientReceiver(void* head);
void* handle_clientSender(void* head);

int main(int argc, char**argv)
{	 char buf1[ MAXMSG]="hello message from client1\n";
	int sockfd;
	struct sockaddr_in servaddr;
	pthread_t Sthread;
	pthread_t Rthread;
	// char buffer[1000];
	
	if (argc != 2)
	{
		printf("usage:  ./%s <IP address>\n",argv[0]);
		return -1;
	}
	/* socket to connect */
	sockfd=socket(AF_INET,SOCK_STREAM,0);

	/* IP address information of the server to connect to */ 
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(argv[1]);
	servaddr.sin_port=htons(32000);
	
	connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	client_tSender* headS=malloc(sizeof(client_tSender));
	headS->sockfd=sockfd;
	headS->buf=malloc(sizeof(char)* MAXMSG);
	strcpy(headS->buf,buf1);
	
	headS->servaddr=servaddr;
	client_tReceiver* headR=malloc(sizeof(client_tReceiver));
	headR->sockfd=sockfd;
	headR->servaddr=servaddr;
	headR->buffer=malloc(sizeof(char)* MAXMSG);
	pthread_create( &Sthread, NULL,handle_clientSender,(void*)headS);
	pthread_create( &Rthread, NULL,handle_clientReceiver,(void*)headR);
	pthread_join(Sthread,NULL);
	pthread_join(Rthread,NULL);
	return 0;
}



void* handle_clientSender(void* head){
	while(1){
	//printf("wait for send...\n");
	client_tSender* newC=(client_tSender*)head;
	
	sendto(newC->sockfd,newC->buf, MAXMSG,0, (struct sockaddr *)&(newC->servaddr),sizeof(newC->servaddr));
	
	sleep(3);
	}

}
void* handle_clientReceiver(void* head){
	int n;
	while(1){
	//printf("getting data from server...\n");
	client_tReceiver* newC=(client_tReceiver*)head;
	n=recvfrom(newC->sockfd,newC->buffer, MAXMSG,0,NULL,NULL);
	newC->buffer[n]=0;
	printf("%s\n",newC->buffer);
	}

}
