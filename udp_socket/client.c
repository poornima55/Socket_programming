#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<unistd.h>
#include<strings.h>
#include<string.h>

void error(char *s)
{
	printf("%s\n",s);
}

int main(int argc, int *argv[])
{
	int sock,length,n;
	struct sockaddr_in server,from;
	struct hostent *hp;
	char buf[256];
	char file_buf[256];
	// checking if hostname and the port address is provided //
	if(argc!=3)
	{
		printf("insufficient arguments\n");
		exit(1);
	}

	//create a socket//
	sock = socket(AF_INET,SOCK_DGRAM,0);

	if(sock<0)
	{
		error("error in opening socket\n");
	}
	
	//to get  the hostname of the system or the machine//
	hp= gethostbyname(argv[1]);

	if(hp==0)
	{
		error("Unknown host\n");
	}
	//build the server's IP address //
	bzero((char *)&server,sizeof(server));
	bcopy((char*)hp->h_addr,(char *)&server.sin_addr,hp->h_length);
	server.sin_family = AF_INET;
	server.sin_port =  htons(atoi(argv[2]));
	length = sizeof(server);
	//get the message from the user//
	printf("enter the message now\n");
	
	scanf("%s",buf);


	//sending message  to server//
	n= sendto(sock,buf,strlen(buf),0,&server,sizeof(struct sockaddr));
	if(n<0)
	{
		error("error in sending message to the server");
	}
	
	//getting reply from the sever//



	memset(buf,'\0',strlen(buf));
	printf("\nprinting the buffer after clearing :%s",buf);
	n=recvfrom(sock,buf,256,0,&server,&length);
	

	printf("\nprinting the data received from the server:%s only",file_buf);
	if(n<0)
	{
		error("error in the recvfrom\n");
	}
	
	FILE *fp;
	fp = fopen("/home/poornima/networking/tmp1.txt","w");
	if(fp==NULL)
	{
		printf("error in handling the file");
		return 1;
	}
	//printing the reply from the server to the designated file//
	if(fwrite(file_buf,1,n,fp)<0)
	{
		printf("error in writing the file\n");
		exit(1);
	}
	
	//close()api tries to complete the transmission if there is still data waiting to be transmitted //
	close(sock);
}


	


	
