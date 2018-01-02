#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<netdb.h>
#include<stdlib.h>
#include<strings.h>
#include<unistd.h>
#include<string.h>

void error(char *s)  /*function for  printing the error*/
{
	printf("%s\n",s);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sock;//serverr socket//
	int  length,fromlen,n;
	struct sockaddr_in server;//server address//
	struct sockaddr_in from;//client address//
	char buf[2000];//message buffer//
	char file_buffer[256];//size of the file buffer//
	//if port number is not specified print error//

	if(argc<2)
	{
		fprintf(stderr, "no port number specified\n");
		exit(0);
	}
	//create a socket//
	sock = socket(AF_INET,SOCK_DGRAM,0);
	if(sock<0)
	{
		error("error in opening socket\n");
	}

	//build the server address//
	length = sizeof(server);
	bzero(&server,length);
	server.sin_family= AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(atoi(argv[1]));

	//bind the socket to the server's address//
	if(bind(sock,(struct sockaddr*)&server,length)<0)
	{
		error("cannot bind\n");
	}

	//size of the client's address//
	fromlen  =sizeof(struct sockaddr_in);

	//wait for the datagram and reply when the  datagram is received// 
	while(1)
	{
		//receive message from the client//
		n = recvfrom(sock,buf,2000,0,(struct sockaddr *)&from,&fromlen);
                // message received from the client is the name of the file//

		printf("message received is %s\n",buf);  	
		FILE *fp;
		fp = fopen(buf,"r"); //the server checks if the file exists or not//
		if(fp==NULL)
		{
			printf("file does not exist\n"); 
		
		}
		fseek(fp,0,SEEK_END); //if exists read the size of the file 
		size_t file_size = ftell(fp);
		fseek(fp,0,SEEK_SET); 
		//start reading the file from the beginning//
		if(fread(file_buffer,file_size,1,fp)<=0)
		{
			printf("error in copying the file");
			exit(1);
		}
		//send the contents of the file  to the client//

		if(sendto(sock,file_buffer,strlen(file_buffer),0,(struct sockaddr*)&from,fromlen)<0)
		{
			printf("error in sending the file\n");
			exit(1);
		}
		printf("data sent to the client is : %s",file_buffer);

		printf("success\n");

		//clear the buffer at the end of each transmission//
		bzero(file_buffer,strlen(file_buffer));
	}
 
    close(sock);
}
	

