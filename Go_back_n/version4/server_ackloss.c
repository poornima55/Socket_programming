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

unsigned short int checksum_2(void  *buffer,unsigned int len,unsigned short int seed)
{
	unsigned char *buf1 = (unsigned char*)buffer;
	int i;

	for(i=0;i<len;i++)
	{
		seed+=(unsigned int)(*buf1++);
	}
                	//fold  32-bit sum to 16 bit
	while(seed>>16)
		seed = (seed & 0xFFFF)+ (seed>>16);
	return seed;
}


int main(int argc, char *argv[])
{
	int num;          //variale used to implement random data loss
	int sock; //server socket
	int length,fromlen,n;
	struct sockaddr_in server; //server address
	struct sockaddr_in from;  //client address
	char buf[65535] = {0, }; //message buffer//
	void* buff =NULL;
	void* file_buf=NULL; //buffer for the file i/o//
	int packets = 0; //number of packets in total to be coming from the client //
	int received = 0; // number of packets received by the server//
	FILE *newfp; // a new file  to store data//

        unsigned short int len;

	int prev;
	int  seq_no = 0;      //sequence number of the current packet
	char data_checksum[65535] = {0, };  //checksum of the data packet received in the string format
	unsigned short int data_checksum_value = 0; //checksum of the data packet received in the integer form
        

	char  data_check_len[1024] = {0, };   //length of the checksum value of the data packet received in the string form
        unsigned short int  data_checksum_len = 0; //length of the checksum of the packet received in the intform
	
	char bytes_length[1024] ={0, };  //length of data bytes in string format
	unsigned short int bytes = 0; //length of data in integer form


	unsigned short int real_checksum = 0;  //the actual checksum of the data received

	char data_content[75535] = {0, }; //the actual payload information ie the data contained in the packet

	char ack_packet[65535] = {0, }; //the data of the  acknowledgement packet 

	int i = 0,j = 0;

	unsigned short int length1; // length of the string variables
        
	char new_ack[65535] = {0, };  // a new ack string to contain the new value of the ack data after randomly corruppting

	unsigned short int ack_checksum = 0;  //the checksum of the ack data

	char ack_checksum_info[65535];  //the checksum info of ack packet in the string form

	unsigned short int ack_check_len;  //length of the checksum of ack packet

	char ack_check_len_str[1024]; //length of the checksum in the string form

	char reply[65535]; //the final reply  that  includes both header and the payload
        
	char* temp; //to hold the temporary string after randomly corrupting the ack

	unsigned short int data_len;
	char data_len_info[1024] = {0};
	char seq_no_char[10] = {0,};

	char seq_char[10] = {0,};
        int seqlen;
	char seqlen_char[10] = {0,};

	char* data = NULL;

	int N = 10;

	newfp = fopen("cupid_home.jpg","wb"); // open the new file in write mode//
	buff = calloc(1,1024); //allocating memory for message buffer
	if(buff==NULL)
	{
		printf("1 memory allocation failed\n");
		return 1;
	}
	file_buf = calloc(1,1024); //allocating memory for file buffer
	data =  calloc(1,65535); //allocating memory for data
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

	if(data== NULL)
	{
		printf("3 memory allocation failed\n");
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
        n = recvfrom(sock,buff,1024,0,(struct sockaddr *)&from,&fromlen);
	if(n<0)
	{
			printf("recvfrom  error\n");
			return 1;
	}
	
	//print the total number of packets to expect from the client//
	packets = atoi((char*)buff);
	printf("Num packets expected: %d\n", packets);
	int ack_seq = 1;
	int last_recv_ok;
	int ack_seq_len;
	char ack_seqlen_char[8] = {0,};

	int req = 1;
	// receive the packets until the total number of packets are arrived//
	while(received<packets+1)
	{
	        memset (buf, 0, 65535);
		memset(data_check_len,0,1024);
		memset(data_checksum,0,65535);
		memset (data_content, 0, 75535);
		memset(bytes_length,0,1024);
	        memset(seqlen_char,0,10);
		memset(seq_no_char,0,10);
		// receive the packets and assemble them in the order//
		n = recvfrom(sock,buf,65535,0,(struct sockaddr *)&from,&fromlen);	

		if(n<0)
		{
			printf("recvfrom  error\n");
			return 1;
		}
		
	
		if (received == packets) {
		  
			// check if its the last packet;
			if(strcmp(buf,"Finish")==0)
		        {
			  printf ("Recieved finish packet\n");
			// if so, 
		            // either break or recieved++ and continue
			   break;
			}

			// else
			    // proceed down (i.e. send ack to the recieved packet)
			//NOTE: else condition may not be needed as we only need to 
			//handle the finishing packet. If its not a finishing packet
			//then handle it like a regular packet by going ahead with the
			//execution of the code below
		}
 

 
               num = 100; //check the number generated to see if data has been lost or not
               //let there be no loss in this case
		if(num>=5)  //here there will not be any loss
	       {
                    	  
                j=0;
                for(i=0;i<1;i++)
		{
	       	seqlen_char[j]=buf[i];   //extract the length of  sequence number of the packet received
                j++;		
		}
		seqlen_char[j] = '\0';


		seqlen = atoi(seqlen_char);//convert the length of the sequence number to integer form

		j=0;
		for(i=1;i<1+seqlen;i++)  //extract the seqence number
		{
			seq_no_char[j]= buf[i];
			j++;
		}
		seq_no_char[j] = '\0';

		seq_no = atoi(seq_no_char);


		printf("The sequence number is %d\n",seq_no);
		printf("The expected number is %d\n",req);
         
	        if(seq_no == req)   //if the sequence number is equal to the expected one then proceed
		{	
		     
	             printf ("Sequence number %d matches with expected value %d\n",
				seq_no, req);

		     j= 0;
		     for(i=1+seqlen;i<2+seqlen;i++)              //the next byte will have the length of the checksum of the received packet
	        	{
		             data_check_len[j] = buf[i];   //extracting the length of the checksum
		             j++;


	            	}
		      data_check_len[j] = '\0';    //making it a string by appending a null at the end 
                     
                     
       		      data_checksum_len = atoi(data_check_len); //convert the checksum length info to integer form
		     
		     

		     j=0;
		     for(i=2+seqlen;i<(2+seqlen+data_checksum_len);i++)  // the next bytes will have the checksum value
	        	{
			data_checksum[j] = buf[i];  //extracting them in the string form
		        j++;
		        }
		     
                      data_checksum[j] = '\0';
		     data_checksum_value = atoi(data_checksum);	//converting the checksum value to interger form
		     
                     
                     data_len_info[0]=buf[i];
                     data_len_info[1] = '\0';

		 
                      
		     data_len = atoi(data_len_info);
		     i++;
		   
                     
		     

		     j=0;
		     while(j<data_len)
	             {
                         bytes_length[j] = buf[i];
			 j++;
			 i++;
		     }

		     printf("number of bytes in the actual data packet payload is %s\n",bytes_length);
		     

		   
                     bytes = atoi(bytes_length);

		     printf("number of bytes in the integer form %d\n",bytes);


                     j=0;
		     while(j<bytes)           //extracting the data payload from the received packet
	        	{
			data_content[j] = buf[i];
			i++;
			j++;
		       }

		  
		     data_content[bytes] = '\0';

		  

	             real_checksum = checksum(data_content,bytes); //calculating the actual checksum of the received packet
		
		     if(real_checksum == data_checksum_value) //if the checksum info in the header info and the actual checksum of the received packet then send ack for the current packet


		    {

                        memset(ack_packet,0,65535);
		        memset(ack_checksum_info,0,65535);
		        memset(ack_check_len_str,0,1024);
		        memset (reply, 0, 65535);	
			

                        
		        	
			printf("checksum value of received data is %d\t and the actual checksum of sent data is %d\n",real_checksum,data_checksum_value);

			strcpy(ack_packet, "Message received");  //the acknowledgement data
			length1 = strlen(ack_packet); //calculating the length
			ack_checksum = checksum(ack_packet,length1); //calculating the checksum of the ack packet

			itoa(ack_checksum,ack_checksum_info,10); //converting the actual checksum value of ackpacket to string form


			ack_check_len = strlen(ack_checksum_info); //calculating the length of the checksum
			itoa(ack_check_len,ack_check_len_str,10);//converting the checksum length to string form
		


                        itoa(ack_seq,seq_char,10); //converting ack_seq to string format
                        
			ack_seq_len = strlen(seq_char);  //finding the length of the sequence number

			itoa(ack_seq_len,ack_seqlen_char,10); //convert the sequence number length to string
                        
		        strcat(reply,ack_seqlen_char); //append the sequence number length to the reply
			strcat(reply,seq_char); //forming the reply packet i.e append  the sequence number
                        strcat(reply,ack_check_len_str); //append the checksum length
                        strcat(reply,ack_checksum_info); //append the checksum value
			strcat(reply,ack_packet); //append the payload data at the end
		     
                        printf("Ack sent to the  packet %d\n",ack_seq);
			printf("Checksum of the ACK sent is %d\n",ack_checksum);
			if(sendto(sock,reply,1024,0,(struct sockaddr*)&from,fromlen)<0) //send the reponse
			{
				printf("error in sending ack to the current packet");
				return 1;
			}
			
	
	
		//write the content of the buffer to the new file//
         		if((fwrite(data_content,1,bytes,newfp)) < bytes)
	        	{
			printf("error in writing to the file\n");
			return 1;
	        	}
		        received++;
		     

		        last_recv_ok = ack_seq;

			ack_seq++;
			req = (req %N)+1;
			

		    }
                	//if the packet received was corrput then send ack to the last received packet ok
		else
		{

		


		        printf("checksum mismatch\n");	
			printf("checksum value of received packet is %d\t and the actual checksum of sent packe is %d\n",real_checksum,data_checksum_value);

                        printf("Ack sent for the last received packet %d\n",last_recv_ok);
			if(sendto(sock,reply,1024,0,(struct sockaddr*)&from,fromlen)<0) //send the reply
			{
				printf("error in sending ack to the last received packet ok");
				return 1;
			}


		}





		}
	//if the packet received does not have expected seqno then send ack to the last received packet ok
		else
		{


                       
			printf ("sequence number %d does NOT match with expected value %d\n",
				seq_no, req);
			printf("Last received packet is %d\n", last_recv_ok);

                        printf("Ack sent for the last received packet\n");
			if(sendto(sock,reply,1024,0,(struct sockaddr*)&from,fromlen)<0) //send the reply
			{
				printf("error in sending ack to the last received packet ok");
				return 1;
			}			
			


		}

	       }

	      else            //if the data loss has occurred , then continue waiting for next packet
	      {
		      printf("data has been lost\n");
		      
	       }

		
		

	}
	//print the message finished at the end of the reception//
	printf("Finished\n");
	//close the file which was being written//
	fclose(newfp);

	//close the socket
	close(sock);
	return 0;
}


	
