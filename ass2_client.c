#include <stdio.h>
#include <stdlib.h>       
#include <unistd.h>  	   //fork
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>  //read write
#include <netdb.h> 	 //hostent structure
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <poll.h>
#include  <signal.h>
#include <fcntl.h> /* Added for the nonblocking socket */
#define BUF_SZ 500
int sockfd;
/*---------------------------------------------------------
|	Function to handle ctrl+c (termination) of client	   |
|	input- int                                             |
|	output- void                                           |
-----------------------------------------------------------*/
void sighandler(int sig_num)
{	
		
		printf("in sig handler\n");
		char buffer[BUF_SZ];
		printf("%c[1A%c[2K\r",27,27);
        sprintf(buffer, "%s", "-1");
        int n = write(sockfd,buffer,BUF_SZ);
		bzero(buffer,BUF_SZ);
        exit(0);
}
int main(){

	int portno = 5001;
	int n;
	char buffer[BUF_SZ];
	char *input = NULL;
	size_t size;
	
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	//fcntl(sockfd, F_SETFL, O_NONBLOCK);
	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT|SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
	if(sockfd == -1) {printf("error:socket formation\n"); return 0;}
	/*server information*/
	struct hostent *server;	
	server = gethostbyname("127.0.0.1");
	struct sockaddr_in servaddr;
	bzero((char *) &servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&servaddr.sin_addr.s_addr, server->h_length);
	servaddr.sin_port = htons(portno);
	if(connect(sockfd,(struct sockaddr *) &servaddr,sizeof(servaddr))== -1) {perror("connect: "); return 0;}
	
	n = read(sockfd,buffer,BUF_SZ);
	if(strcmp(buffer,"CONNECTION LIMIT EXCEEDED\n")==0){
		printf("%s\n", buffer);
		exit(0);
	}
	printf("%s\n", buffer);
	bzero(buffer,BUF_SZ);

	struct pollfd fds[]={
		{sockfd,POLLIN},
		{0,POLLIN}
	};
	signal(SIGINT, sighandler);
	printf("\r%c[2K",27);
	printf("1.broadcast:<msg>     -- TO BROADCAST MSG \n");
	printf("2.showUsers           -- TO DISPLAY ALL CONNECTED CLIENTS \n");
	printf("3.<client_name>:<msg> -- TO MESSAGE A PARTICULAR CLIENT \n");
	printf("4.press ctrl+c        -- TO DISCONNECT CLIENT \n\n");
	while(1){
		poll(fds,2,-1);
		int i;
		for(i=0;i<2;i++){
			if(fds[i].revents & POLLIN ){ 
				if(i==0){
					//read server reply
					n =read(sockfd,buffer,BUF_SZ);
					printf("%s",buffer);
					fflush(NULL);
					bzero(buffer,BUF_SZ);
				}
				else if(i==1){
					//take user input and send to the server
					getline(&input,&size,stdin);
					if(!(strcmp(input,"showUsers\n")==0) && !(strcmp(input,"broadcast\n")==0)){
						printf("%c[1A%c[2K\r",27,27);
						printf("                                     %s\n", input);
					}
					sprintf(buffer, "%s", input);
					n = write(sockfd,buffer,BUF_SZ);
					bzero(buffer,BUF_SZ);
				}
			}
		}
	}
	

	close(sockfd);
	return 0;
}
