#include<stdio.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<arpa/inet.h>

unsigned short int checksum(unsigned char * buff,unsigned int count)
{
	register unsigned int sum = 0;
	//main summing loop
	while(count >1)
	{
		sum +=  *((unsigned short int*) buff);
			(buff)++;
		count = count -2;
	}
        //add left over byte if any
	if(count>0)
	{
		sum += *((unsigned char *)buff);
	}
	//fold  32-bit sum to 16 bit
	while(sum>>16)
		sum = (sum & 0xFFFF)+ (sum>>16);
	return (~sum);
}




char *itoa(long i,char *s,int dummy_radix)
{
	sprintf(s,"%ld",i);
	return s;
}

int main(int argc, int *argv[])
{
	int sock,length,n;
	struct sockaddr_in server,from;
	struct hostent *hp;
	long int packets =0;
	unsigned short int check;
	void* buff = NULL;

	buff = calloc(1,1024);//allocating memory for message buffer
	if(buff == NULL)
	{
		printf("memory allocation failed\n");
		return 1;
	}

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
		printf("error in opening socket\n");
		return 1;
	}
	
	//to get  the hostname of the system or the machine//
	hp= gethostbyname(argv[1]);

	if(hp==0)
	{
		printf("Unknown host\n");
		return 1;
	}
	//build the server's IP address //
	bzero((char *)&server,sizeof(server));
	bcopy((char*)hp->h_addr,(char *)&server.sin_addr,hp->h_length);
	server.sin_family = AF_INET;
	server.sin_port =  htons(atoi(argv[2]));
	length = sizeof(server);
	
	/*open the file that we wish to transfer*/
	FILE *fp = fopen("temp1.txt","rb");
	if(fp==NULL)
	{
		printf("file open error");
		return 1;
	}
	fseek(fp,0,SEEK_END); //if exists read the size of the file 
	size_t file_size = ftell(fp);
	fseek(fp,0,SEEK_SET); 
	
	printf("size of the file is %d\n", file_size);

	/*find the number of packets*/
	if(file_size == 0)
	{
		packets = 0;
	}
	else
	{

	packets = (file_size/1024)+1 ;
	}
	
	/*send the number of packets to the server*/

	 itoa(packets,(char*)buff,10);
	n= sendto(sock,buff,1024,0,(struct sockaddr *)&server,sizeof(struct sockaddr));
        if(n<0)
        {
	       	printf("error in sending message to the server");
		return 1;
	}
	


	
	/*Read data from file and send it*/
	int packetNum = 0;
	while(1)
	{
		/*First read file in chunks of  1024  bytes */

		int nread = fread(buff,1,1024,fp);
		//printf("Bytes read %d\n",nread);

		/*if read was success ,send data*/
		if(nread>0)
		{
			
			check = checksum(buff,1024);
			printf("checksum = %04X\n",check);
			//printf("data sent now is %s\n",buff);
			n= sendto(sock,buff,nread,0,(struct sockaddr *)&server,sizeof(struct sockaddr));
			printf("Sending %d, numBytes sent: %d\n", packetNum, n);
			packetNum++;
	                if(n<0)
                 	{
		              printf("error in sending message to the server");
			      fclose(fp);
			      return 1;
			}
	
		}

		/*There is something tricky going on with the read..
		 * Either there was error ,or we reached end of  file.
		 */
		if(nread==0)
		{
			if(feof(fp))
				printf("End of file\n");
				
			if(ferror(fp))
				printf("Error reading\n");
			break;
		}

		sleep(1); 

	}
	
	fclose(fp); //close the  file to complete the transmission

	close(sock); //close api tries to complete the transmission if there is data waiting to  be transmitted

	return 0;
}
