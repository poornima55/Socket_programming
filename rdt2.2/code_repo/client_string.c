#include<stdio.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<arpa/inet.h>
#include<sys/socket.h>

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

int main(int argc, int *argv[])
{
	int sock,length,n;
	struct sockaddr_in server,from;
	struct hostent *hp;
	long int packets =0;

	unsigned short int check;
	void* buff;
	char buf[65535]={0};
	char new_buff[65535]={0};  
	char ack_buf[65535]={0};
	char* temp;
	char checksum_info[1024]={0} ;
	char prev = '1';
	char seq_no;
	char current;
	char data[75535]={0};
	unsigned  short int check_length;
	char check_len_str[1024]={0};
	char ack_check_len[1024]={0};
        unsigned short int  ack_checksum_len; 	
	char ack_check[65535]={0};
	unsigned short int ack_checksum;
        char temp_buf[65535]={0};
        unsigned short int actual_checksum;
        unsigned short int length1;
	char content[65535]={0};
        int i,j;
	buff = calloc(1,1024);//allocating memory for message buffer
	if(buff == NULL)
	{
		printf("memory allocation failedi\n");
		return 1;
	}

	temp = calloc(1,65535); //allocating memory for new message

	if(temp == NULL)
	{
		printf("memory allocation failed\n");
		return 1;
	}
//
//	ack_buff = calloc(1,256); //allocating memory for acknowledgement from the server


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
	FILE *fp = fopen("temp5.txt","rb");
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
	//packets = 4;
	}
	
	/*send the number of packets to the server*/

	 itoa(packets,(char*)buff,10);
	 printf("packets =%s\n",(char*)buff);

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

		int nread = fread(buf,1,1024,fp);
		printf("Bytes read %d\n",nread);
       		printf("message read is %s\n",buf);        
		/*if read was success ,send data*/
		if(nread>0)
		{
			
			
			 length1 = strlen(buf); //calculate the length  of the buff 
			 //printf("%d\n",length1);
            		 check = checksum(buf,length1);    //calculate the checksum
    			 itoa(check,checksum_info,10);  //convert checksum into string
			 check_length = strlen(checksum_info);  //calculate the length of the checksum
			 itoa(check_length,check_len_str,10); //convert the checksum lenght to string form
			 temp = rand_corrupt(buf);   //randomly corrupt the data
			 strcpy(new_buff,temp);      //assign it to the new_buff
			   
			if(prev == '1')              //assign the sequence number
       			{
				seq_no = '0';
				current = seq_no;
			}
			else
			{
				seq_no = '1';
				current = seq_no;
			}
			if(seq_no == '0')
				prev = '1';
			else
				prev = '0';
			
			               
                        data[0] = seq_no;     //combine seq no,checksum and the data content into one packet 
                        printf("sequence number is %c\n",data[0]);


			strcat(data,check_len_str); //adding length of the checksum
			//printf("length of checksum is %s\n",check_len_str);

			strcat(data,checksum_info); //adding checksum
			//printf("checksum is %s\n",checksum_info);

	                strcat(data,new_buff);   //randomly corrupting data packets and adding them		
			//printf("the new  data is %s\n",new_buff);
                        
       			n= sendto(sock,data,strlen(data),0,(struct sockaddr *)&server,sizeof(struct sockaddr));///send the data packet after adding the header info
			//*printf("sending the data packet %s",data);
			//printf("Sending %d, numBytes sent: %d\n", packetNum, n);
			packetNum++; //increment the packets sent
	                if(n<0)
                 	{
		              printf("error in sending message to the server");
			      fclose(fp);
			      return 1;
			}

			while(1)      //wait for the acknowledgement for the sent packet
			{
			    n =  recvfrom(sock,ack_buf,1024,0,&server, &length); //receive the ack from the server
			   //printf("ACK received is %s\n",ack_buf);
			    seq_no = ack_buf[0];  //first byte will  be sequence number
			    printf("Ack received for the sequence number %c\n",seq_no);
         		    if(seq_no == current)    //if the sequence number matches then proceed 
		             {

				        printf("A positive ACK\n");

			                j =0;
			                for(i=1;i<2;i++)   //next   byte will have the length of checksum
		                      {
				      temp_buf[j] = ack_buf[i];  //extracting the checksum length
				      j++;
		                       }
                                       temp_buf[j] = '\0'; //ending the string with a null character
			              strcpy(ack_check_len,temp_buf); //length of checksum will  be in string format
			              ack_checksum_len =  atoi(ack_check_len); //convert it into int
			               j = 0;
			              for(i=2;i<(2+ack_checksum_len);i++)  //extract the checksum
		                      {

	                                 ack_check[j] = ack_buf[i];
				         j++;
		                       }
				      ack_check[j] ='\0';

			               ack_checksum = atoi(ack_check);  //the checksum will be in string format,convert it into  the integer format
			               j=0;            //extract the content i.e the actual ack message
		               	       while(i<=n)
			           {
				   content[j] = ack_buf[i];
				   i++;
				   j++;
		                    }
				       content[j] ='\0';
				      length1 = strlen(content);
                                  actual_checksum =  checksum(content,length1);//calculate the actual checksum
			          if(actual_checksum == ack_checksum) //if the ack is not corrupt then break the loop and send the next packet
			          {
		                  break;
				  }
			          else    //resend the data packet to the server
		                  {
	                             n = sendto(sock,data,strlen(data),0,(struct sockaddr *)&server,sizeof(struct sockaddr));
                                   continue;
				  }
				}
			else  //even if the sequence number of the ack packet does not match then resend the data packet to the server
			{

                              n = sendto(sock,data,strlen(data),0,(struct sockaddr *)&server,sizeof(struct sockaddr));
                        
                                    continue;
			
                         }

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
