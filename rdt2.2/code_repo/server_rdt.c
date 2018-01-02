#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<netdb.h>
#include<stdio.h>
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

char* rand_corrupt(unsigned char* buff)
{
	int n = strlen(buff);
	char* str_temp = (char*)calloc(1,n); //allocate memory for a string 
	int i;
	int num;

	num = rand() %100;

	if(num>95)
	{
              for(i=0;i<strlen(buff);i++)
	      {
		      buff[i] = buff[i]+1;
		}
	}
	strcpy(str_temp,buff);
	return str_temp;

}

char *itoa(long i,char *s,int dummy_radix)
{
	sprintf(s,"%ld",i);
	return s;
}




int main(int argc, char *argv[])
{
	int sock; //server socket
	int length,fromlen,n;
	struct sockaddr_in server; //server address
	struct sockaddr_in from;  //client address
	char* buf= NULL; //message buffer//
	void* file_buf=NULL; //buffer for the file i/o//
	int packets = 0; //number of packets in total to be coming from the client //
	int received = 0; // number of packets received by the server//
	FILE *newfp; // a new file  to store data//
	//unsigned short int check_rcv;

	char prev = 1;    //sequence number of previous packet received
	char seq_no;      //sequence number of the current packet
	char* data_checksum='\0';  //checksum of the data packet received in the string format
	unsigned short int data_checksum_value; //checksum of the data packet received in the integer form
        

	char*  data_check_len ='\0';   //length of the checksum value of the data packet received in the string form
        unsigned short int  data_checksum_len; //length of the checksum of the packet received in the intform
	
	
	unsigned short int real_checksum;  //the actual checksum of the data received

	char* data_content ='\0'; //the actual payload information ie the data contained in the packet

	char* ack_packet = '\0'; //the data of the  acknowledgement packet 

	int i,j;

	unsigned short int length1; // length of the string variables
        
	char* new_ack;  // a new ack string to contain the new value of the ack data after randomly corruppting

	unsigned short int ack_checksum;  //the checksum of the ack data

	char* ack_checksum_info;  //the checksum info of ack packet in the string form

	unsigned short int ack_check_len;  //length of the checksum of ack packet

	char* ack_check_len_str; //length of the checksum in the string form

	char* reply; //the final reply  that  includes both header and the payload
        
	char* temp; //to hold the temporary string after randomly corrupting the ack
	newfp = fopen("temp2.txt","wb"); // open the new file in write mode//
	//buf = calloc(1,1024); //allocating memory for message buffer
	//if(buf==NULL)
	//{
	//	printf("1 memory allocation failed\n");
	//	return 1;
	//}
	file_buf = calloc(1,1024); //allocating memory for file buffer
	if (!file_buf)
	{
		printf("2 memory allocation failed\n");
		return 1;
	}

	if(newfp==NULL) // error if the file can not open//
	{
		printf("error opening the file\n");
	        return 1;
	}	
	if(argc<2) //print error if no port number is specified//
	{
		fprintf(stderr, "no port number specified\n");

		exit(0);
	}
	sock = socket(AF_INET,SOCK_DGRAM,0);//create a server socket//
	if(sock<0)
	{
		printf("error in opening socket\n");
		return 1;
	}
	//build the server address//
	length = sizeof(server);
	bzero(&server,length);
	server.sin_family= AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(atoi(argv[1]));
	//bind the socket to  the server's address//
	if(bind(sock,(struct sockaddr*)&server,length)<0)
	{
		printf("cannot bind\n");
		return 1;
	}
	//size of the client's address//
	fromlen  =sizeof(struct sockaddr_in);
	// receive the first packet from the client which indicates total number of packets//
        n = recvfrom(sock,buf,1024,0,(struct sockaddr *)&from,&fromlen);
	if(n<0)
	{
			printf("recvfrom  error\n");
			return 1;
	}
	
	//print the total number of packets to expect from the client//
	packets = atoi((char*)buf);
	printf("Num packets expected: %d\n", packets);
	
	// receive the packets until the total number of packets are arrived//
	while(received<packets)
	{
		// receive the packets and assemble them in the order//
		n = recvfrom(sock,buf,1024,0,(struct sockaddr *)&from,&fromlen);	
		
	        
	  
		if(n<0)
		{
			printf("recvfrom  error\n");
			return 1;
		}

		seq_no=buf[0];   //extract the sequence number of the packet received
		if(seq_no == '0')  //note down the sequence number of the previous packet recived
			prev = '1';
		else
			prev ='0';

		j= 0;
		for(i=1;i<=4;i++)              //the next 4 bytes will have the length of the checksum of the received packet
		{
		 data_check_len[j] = buf[i];   //extracting the length of the checksum
		 j++;
	       	}
		data_check_len[j] = '\0';    //making it a string by appending a null at the end 
       		data_checksum_len = atoi(data_check_len); //convert the checksum length info to integer form
		
		j=0;
		for(i=4;i<(4+data_checksum_len);i++)  // the next bytes will have the checksum value
		{
			data_checksum[j] = buf[i];  //extracting them in the string form
		        j++;
		}
		data_checksum[j] = '\0';  //ending the string by appending a NULL character

		data_checksum_value = atoi(data_checksum);	//converting the checksum value to interger form

                j=0;
		while(j<n)                          //extracting the data payload from the received packet
		{
			data_content[j] = buf[i];
			i++;
			j++;
		}

		data_content[j] ='\0';  //ending the string with a null character

                length1 = strlen(data_content); //length of the string 

	        real_checksum = checksum(data_content,length1); //calculating the actual checksum of the received packet

		if(real_checksum == data_checksum_value) //if the checksum info in the header info and the actual checksum of the received packet then send ack for the current packet


		{
			ack_packet = "Message received\n";  //the acknowledgement data
			length1 = strlen(ack_packet); //calculating the length
			ack_checksum = checksum(ack_packet,length1); //calculating the checksum of the ack packet
			
			temp = rand_corrupt(ack_packet); //intentionally corrupt the packet using the rand_corrupt function  

			strcpy(new_ack,temp); //copying the temp string to the new ack 

			itoa(ack_checksum,ack_checksum_info,10); //converting the actual checksum value of ackpacket to string form
			ack_check_len = strlen(ack_checksum_info)-1; //calculating the length of the checksum
			itoa(ack_check_len,ack_check_len_str,10);//converting the checksum length to string form
			
			reply[0] = seq_no; //forming the reply packet i.e append  the sequence number
                        strcat(reply,ack_check_len_str); //append the checksum length
                        strcat(reply,ack_checksum_info); //append the checksum value
			strcat(reply,new_ack); //append the payload data at the end 
                        
			if(sendto(sock,reply,strlen(reply),0,(struct sockaddr*)&from,fromlen)<0) //send the reponse
			{
				printf("error in sending ack");
				return 1;
			}

		}

		//if the packet received was corrput then send ack to the last received packet ok
		else
		{
			ack_packet = "Message received\n";  //actual  ack message
			length1  = strlen(ack_packet); //calculating the length of the ack
			ack_checksum = checksum(ack_packet,length1); //calculating the checksum of the ack packet
			
 		 	itoa(ack_checksum,ack_checksum_info,10); //converting the checksum to string form
			ack_check_len = strlen(ack_checksum_info)-1;//calculating the length of the checksum
			itoa(ack_check_len,ack_check_len_str,10);//converting the checksum length to string form 
			
			reply[0] = prev; //form the reply, append the sequence number of the last received packet
                        strcat(reply,ack_check_len_str); //append the length of the checksum

                        strcat(reply,ack_checksum_info); //append the value of the checksum of the ack packet

			strcat(reply,ack_packet); //append the payload ack message

                        
			if(sendto(sock,reply,strlen(reply),0,(struct sockaddr*)&from,fromlen)<0) //send the reply
			{
				printf("error in sending ack");
				return 1;
			}


		}
		
		//write the content of the buffer to the new file//
		if((fwrite(buf,1,n,newfp)) < n)
		{
			printf("error in writing to the file\n");
			return 1;
		}
		received++;

	}
	//print the message finished at the end of the reception//
	printf("Finished\n");
	//close the file which was being written//
	fclose(newfp);

	//close the socket
	close(sock);
	return 0;
}


	
