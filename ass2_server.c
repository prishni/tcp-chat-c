#include <stdio.h>
#include <stdlib.h>        /*fork*/
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>    /*read, write, sockaddr*/
#include <netdb.h> 	       /*To store server info in clinet's process, hostent struct*/
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <poll.h>
#include  <signal.h>
#include <fcntl.h>        /* Added for the nonblocking socket */
#define BUF_SZ 500        /*size of buffer*/
#define MAX_CLIENTS 5     /*max number of clients that can be connected*/

/*---------------------------------------------------------
|   structure to store all connected clients information   |
-----------------------------------------------------------*/
struct clientInfo{
	int id;
	char name[30];
	int fd;
	time_t timestamp;
};
/*----------------------------------------------------------
|	structure to store messages       	           |
-----------------------------------------------------------*/
struct message{
	int s_id,r_id;
	char s_name[30];
	char r_name[30];
	char msg[BUF_SZ];
};
/*---------------------------------------------------------
|	Function to handle ctrl+c (termination) of server	   |
|	input- int                                             |
|	output- void                                           |
-----------------------------------------------------------*/
void sighandler(int sig_num)
{	
		char buffer[BUF_SZ];
		printf("\r%c[2K",27);
        sem_unlink("/qsem");
        exit(0);
}
/*--------------------------------------------------------------
|	generates a randon number in the range [0,RAND_MAx]    |
|	input- NULL                                            |
|	output- int                                            |
----------------------------------------------------------------*/
int generateClientid(){
	srand ( time(NULL) );
  	int random_number = rand();
  	return random_number;
}
/*--------------------------------------------------------------
|	checks if there is message for client in yhe queue     |
|	input- msg queue,front pointer, client name(self)      |
|	output- int                                            |
----------------------------------------------------------------*/
int checkQueue(struct message *msg_queue,int *front,char *myCliName){
	struct message m = msg_queue[*front];
	if(strcmp(m.r_name,myCliName)==0)
		return 1;
	return 0;
}
/*--------------------------------------------------------------
|	checks if the destination client exixts                |
|	input- destination client's name, array of all         |
|		   connected clients                               |
|	output- int (1 if client exists, 0 otherwise)          |
----------------------------------------------------------------*/
int checkIfSrcExists(char *dest_name,int no_of_server,struct clientInfo clients[]){
	for(int i=1;i<=no_of_server;i++){
		if((strcmp(dest_name,clients[i].name)==0) && clients[i].id !=-1)
			return 1;
	}
	return 0;
}
/*--------------------------------------------------------------
|	checks if the clients tries to ping itself             |
|	input- destination client's name, source client's      |
|		   name                                            |
|	output- int (1 if client pings itself, 0 otherwise)    |
----------------------------------------------------------------*/
int ifMsgToItself(char *dest_name,char *src_name){
	if(strcmp(dest_name,src_name)==0) return 1;
	return 0;
}
/*--------------------------------------------------------------
|	dequeues message form queue      	                   |
|	input- front & rear pointers, msg queue                |
|	output- dequeued message                               |
----------------------------------------------------------------*/
struct message dequeue(int *front, int *rear, struct message *msg_queue){
	struct message msg ;
	if(!(*front > *rear || *front == 100 || *front == -1)){
		msg = msg_queue[*front];
		if(*front == *rear){
			*front = *rear =-1;
		}
		else{
			(*front)++;
		}
	}
	return msg;
}
/*--------------------------------------------------------------
|	equeues a message     	                               |
|	input- front & rear pointers, msg queue ,message  m    |
|	output- void                                           |
----------------------------------------------------------------*/
void enqueue(int *front, int *rear,  struct message *msg_queue,struct message m,FILE *fp){
	if(*rear ==99) return;
	else{
		msg_queue[(*rear)+1] = m;
		time_t tm =time(NULL);
		fprintf(fp, "Sender: %s , Receiver: %s , Time Stamp %s Message : %s",m.s_name,m.r_name,ctime(&tm),m.msg);
		(*rear)++;
		if(*front == -1) *front =0;
	}
}
/*--------------------------------------------------------
|	Display all available users/clients to a client  |
|	Input- NULL                                      |
|	Output- client information                       |
---------------------------------------------------------*/
void showUsers(struct clientInfo clients[],int no_of_server,int newsock_fd,int myid){
	int n;
	char buffer[BUF_SZ];
	bzero(buffer,BUF_SZ);
	char input[BUF_SZ];
	int i,flag=0;
	for(i=1;i<=no_of_server ;i++){
		if(clients[i].id!=-1 && clients[i].id != myid){
			sprintf(input,"  %d. CLIENT%d:-\n  ID= %d\n  NAME= %s\n\n",i,i,clients[i].id,clients[i].name);
			strcat(buffer,input);
			bzero(input,BUF_SZ);
			flag=1;
		}
	}
	if(flag==0){
		sprintf(buffer,"%s","NO OTHER CLIENTS CONNECTED\n");
		n = write(newsock_fd,buffer,BUF_SZ);
		bzero(buffer,BUF_SZ);
	}
	else{
		n = write(newsock_fd,buffer,BUF_SZ);
		bzero(buffer,BUF_SZ);
		sprintf(buffer,"----------------------------------------------------------------\n",BUF_SZ);
		n = write(newsock_fd,buffer,BUF_SZ);
		bzero(buffer,BUF_SZ);
	}
}
/*---------------------------------------------------------
|   broadcasts "disconnecting" msg to all connected      |
|   clients when any client disconnects                  |
|   Input- array of connected clients,front & rear       |
|		   pointers, msg_queue                           |
|    Output- void                                        |
----------------------------------------------------------*/
void broadcast(struct clientInfo clients[],int no_of_server,char *myCliName,int *front,int *rear,struct message *msg_queue,char *msg,int myid,FILE *fp){
	struct message m;
	strcpy(m.s_name,myCliName);
	strcpy(m.msg,msg);
	for(int i=1;i<=no_of_server;i++){
		if((clients[i].id != -1) && (clients[i].id != myid) ){
			strcpy(m.r_name,clients[i].name);
			enqueue(front,rear,msg_queue,m,fp);
		}
	}
}

int main(){

	int port_no = 5000;
	int sock_fd = socket(AF_INET,SOCK_STREAM,0);
	if(sock_fd ==-1){printf("error:socket formation\n"); return 0;}
	fcntl(sock_fd, F_SETFL, O_NONBLOCK);
	/*server information*/
	struct sockaddr_in servaddr;
	bzero((char *)&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr= INADDR_ANY;
	servaddr.sin_port = htons(port_no);
	if(bind(sock_fd,(struct sockaddr*)&servaddr,sizeof(servaddr)) <0);
	listen(sock_fd,5);
	
	/*client information*/
	struct sockaddr_in clientaddr;
	socklen_t c_len = sizeof(clientaddr);

	int shmid = shmget(IPC_PRIVATE,sizeof(int)*1, IPC_CREAT | 0666);
	int *front =(int *)shmat(shmid,NULL,0);
	shmid = shmget(IPC_PRIVATE,sizeof(int)*1, IPC_CREAT | 0666);
	int *rear =(int *)shmat(shmid,NULL,0);
	shmid = shmget(IPC_PRIVATE,sizeof(clientInfo)*5, IPC_CREAT | 0666);
	struct clientInfo *clients =(struct clientInfo*)shmat(shmid,NULL,0);
	shmid = shmget(IPC_PRIVATE,sizeof(message)*100, IPC_CREAT | 0666);
	struct message *msg_queue =(struct message *)shmat(shmid,NULL,0);
	shmid = shmget(IPC_PRIVATE,sizeof(int)*1, IPC_CREAT | 0666);
	int *no_of_server =(int *)shmat(shmid,NULL,0);
	*no_of_server=0;
	*front =-1;
	*rear =-1;
	sem_t *qsem = sem_open("/qsem",O_CREAT|O_EXCL,0644,1);
	FILE *fp =fopen("serverlog.txt","w");
	signal(SIGINT, sighandler);
	for(;;)
	{
		int newsock_fd = accept(sock_fd,(struct sockaddr *)&clientaddr , &c_len );
		if(newsock_fd ==-1){continue;}
		fcntl(newsock_fd, F_SETFL, O_NONBLOCK);
		char buffer[BUF_SZ];
		struct pollfd fds[]={
			{newsock_fd,POLLIN},
			{0,POLLIN}
		};

		/* If 5 clients are already connected than prohibit further connections*/
		if(*no_of_server >= MAX_CLIENTS){
			sprintf(buffer, "%s","CONNECTION LIMIT EXCEEDED\n");
			int n = write(newsock_fd,buffer,BUF_SZ);
			bzero(buffer,BUF_SZ);
			close(newsock_fd);
		}
		else{ 
			(*no_of_server)++;
			
			pid_t pid =fork();
			if(pid == -1){
				close(newsock_fd);
				continue;
			}
			if(pid > 0){							//Parent
				close(newsock_fd);
				continue;
			}
			if(pid == 0){							//Child
				int n;
				int myclient = *no_of_server;
				char myCliName[30] ="client";
				clients[*no_of_server].id=generateClientid()%100;
				char temp[5];
				sprintf(temp,"%d",myclient);
				strcat(myCliName,temp);
				strcpy(clients[*no_of_server].name,myCliName);
				clients[*no_of_server].fd=newsock_fd;
				clients[*no_of_server].timestamp= time(NULL);
				sprintf(buffer,"\n WELCOME, YOU ARE CONNECTED \n ID = %d\n NAME = %s \n TIME STAMP = %s ----------------------------------------------------------------\n",clients[*no_of_server].id,clients[*no_of_server].name ,ctime(&clients[*no_of_server].timestamp));
				n = write(newsock_fd,buffer,BUF_SZ);
				bzero(buffer,BUF_SZ);
				printf("A client connected with\n Id = %d\n Name = %s \n TimeStamp = %s\n",clients[*no_of_server].id,clients[*no_of_server].name ,ctime(&clients[*no_of_server].timestamp));
				//fprintf(fp,"A client connected with\n Id = %d\n Name = %s \n TimeStamp = %s\n",clients[*no_of_server].id,clients[*no_of_server].name ,ctime(&clients[*no_of_server].timestamp));
				while(1){
						n = read(newsock_fd,buffer,BUF_SZ);
						//if client has provided data on cmd
						if(n>0){
							if(strncmp("showUsers\n",buffer,BUF_SZ)==0){
								bzero(buffer,BUF_SZ);
								showUsers(clients,*no_of_server,newsock_fd,clients[myclient].id);
							}
							else if(strncmp("broadcast:",buffer,strlen("broadcast:"))==0){
								char msg[BUF_SZ];
								sprintf(msg,"%s\n",buffer+strlen("broadcast:"));
								broadcast(clients,*no_of_server,myCliName,front,rear,msg_queue,msg,clients[myclient].id,fp);
								bzero(buffer,BUF_SZ);
							}
							else if(strncmp("-1",buffer,BUF_SZ)==0){
								printf("%s with id %d is DISCONNECTED\n\n",myCliName,clients[myclient].id);
								//fprintf(fp,"%s with id %d is disconnected\n\n",myCliName,clients[myclient].id);
								clients[myclient].id =-1;
								bzero(buffer,BUF_SZ);
								char msg[30]= "IS DISCONNECTED\n";
								broadcast(clients,*no_of_server,myCliName,front,rear,msg_queue,msg,clients[myclient].id,fp);
								close(sock_fd);
								return 0;
							}
							else{
								if(!strchr(buffer,':') ){
									//printf("input -%s",buffer );
									sprintf(buffer,"%s","                                     PLZ PEOVIDE INPUT IN THIS FORMAT client_name:<msg>\n");
									write(newsock_fd,buffer,BUF_SZ);
									bzero(buffer,BUF_SZ);
								}
								else{
									int index= strcspn(buffer,":");
									if(index==0 && strchr(buffer,':')  ){
										sprintf(buffer,"%s","                                     PLZ PROVIDE A CLIENT NAME\n");
										write(newsock_fd,buffer,BUF_SZ);
										bzero(buffer,BUF_SZ);
									}
									else{
										struct message m;
										char *src_name = myCliName;
										char dest_name[30],msg[BUF_SZ];
										memcpy(dest_name,&buffer,index);
										sprintf(msg,"%s",buffer+index+1);
										//msg[strlen(msg)-1]='\0';
										dest_name[index]='\0';
										strcpy(m.msg,msg);
										strcpy(m.s_name,src_name);
										strcpy(m.r_name,dest_name);
										m.s_id=-1;
										m.r_id=-1;
										
										if(ifMsgToItself(dest_name,src_name)){
											sprintf(buffer,"%s","                                     YOU CANNOT PING YOURSELF\n");
											write(newsock_fd,buffer,BUF_SZ);
											bzero(buffer,BUF_SZ);
										}
										else if(checkIfSrcExists(dest_name,*no_of_server,clients)){
											sem_wait(qsem);
											enqueue(front,rear,msg_queue,m,fp);
											sem_post(qsem);
										}
										else{
											sprintf(buffer,"%s","                                     THIS CLIENT DOESNOT EXISTS OR IS NO MORE ONLINE\n");
											write(newsock_fd,buffer,BUF_SZ);
											bzero(buffer,BUF_SZ);
										}
									}
								}
							}
						}
						//if client has not provided any command
						else {
							sem_wait(qsem);
							if(checkQueue(msg_queue,front,myCliName)){
								struct message m =dequeue(front,rear,msg_queue);
								sprintf(buffer,"%s: %s",m.s_name, m.msg);
								write(newsock_fd,buffer,BUF_SZ);
								bzero(buffer,BUF_SZ);
							}
							sem_post(qsem);
						}
				}
				break;
			}	
		}
	}
	close(sock_fd);
	fclose(fp);
	return 0;
}