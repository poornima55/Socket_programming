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

char* rand_corrupt(unsigned char* buff)
{
	int n = strlen(buff);
	char* str_temp = (char*)calloc(1,n); //allocate memory for a string 
	int i;
	int num;

	num = rand() %100;

	if(num>50)
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
	void* buff = NULL;
	void* new_buff =  NULL ;  
	void* ack_buff =  NULL;
	char* temp;
	char* checksum_info = "\0";
	char prev = '1';
	char seq_no;
	char current;
	char* data = "\0";
	unsigned  short int check_len;
	char* check_len_str;
	char* ack_check_len;
        unsigned short int  ack_checksum_len; 	
	char* ack_check;
        char* temp_buf;
        unsigned short int actual_checksum;
    
	buff = calloc(1,1024);//allocating memory for message buffer
	if(buff == NULL)
	{
		printf("memory allocation failedi\n");
		return 1;
	}

	new_buff = calloc(1,1024); //allocating memory for new message

	if(new_buff == NULL)
	{
		printf("memory allocation failed\n");
		return 1;
	}

	ack_buff = calloc(1,256); //allocating memory for acknowledgement from the server


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
			
			check = checksum(buff,1024);    //calculate the checksum

			printf("checksum = %04X\n",check); //print the checksum value
			
			 itoa(check,checksum_info,10);  i//convert checksum into string
			 check_length = strlen(checksum_info)-1;
			 itoa(check_length,check_len_str,10);
			 temp = rand_corrupt(buff);   //randomly corrupt the data
			 strcpy(new_buff,temp);      //assign it to the new_buff
			 prev = current;  //note down the sequence number
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
			
			               
                        strcpy(data,seq_no);     //combine seq no,checksum and the data content into one packet 
			strcat(data,check_len_str); //adding length of the checksum
			strcat(data,checksum_info); //adding checksum
	                strcat(data,new_buff);   //randomly corrupting data packets and adding them		
       			n= sendto(sock,data,nread,0,(struct sockaddr *)&server,sizeof(struct sockaddr));
			printf("Sending %d, numBytes sent: %d\n", packetNum, n);
			packetNum++;
	                if(n<0)
                 	{
		              printf("error in sending message to the server");
			      fclose(fp);
			      return 1;
			}

			while(1)
			{
			    n =  recvfrom(sock,ack_buf,1024,0,&server, &length); //receive the ack from the server
			    seq = ack_buf[0];  //first byte will  be sequence number
			    if(seq == current)    //if the sequence number matches then proceed 
		               {

			                j =0;
			                for(i=1;i<=4;i++)   //next 2 bytes will have the length of checksum
		                      {
				      temp_buf[j] = ack_buf[i];
				      j++;
		                       }
                                       temp_buf[j] = '\0';
			              strcpy(ack_check_len,temp_buf); //length of checksum will  be in string format
			              ack_checksum_len =  atoi(ack_check_len); //convert it into int
			              j = 0;
			              for(i=4;i<(4+ack_checksum_len);i++)  //extract the checksum
		                      {

	                                 ack_check[j] = ack_buf[i];
				         j++;
		                       }
				      ack_check[j] ='\0';

			               ack_checksum = atoi(ack_check);  //the checksum will be in string format,convert it into  the integer format
			               j=0;                           //extract the content
		               	       while(j<n)
			           {
				   content[j] = ack_buf[i];
				   i++;
				   j++;
		                    }
				       content[j] ='\0';
                                  actual_checksum =  checksum(content,n);
			          if(actual_checksum == ack_checksum)
			          {
		                  break;
				  }
			          else
		                  {
	                             n = sendto(sock,data,nread,0,(struct sockaddr *)&server,sizeof(struct sockaddr));
                          
                                    continue;
			
		       		  }
			else
			{

                              n = sendto(sock,data,nread,0,(struct sockaddr *)&server,sizeof(struct sockaddr));
                          
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
