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
#include<time.h>
#include<sys/time.h>
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

char* rand_corrupt(char* buff,unsigned short int bytes)
{
	
	char* str_temp = (char*)calloc(1,bytes+1); //allocate memory for a string 
	if(str_temp==NULL)
	{
		printf("error\n");
	}
	int i;
	int num;

	num = rand() %100;

	if(num<30)              //30% error rate
	{
              for(i=0;i<bytes;i++)
	      {
		      buff[i] = buff[i]+'1';
		}
	}
	
	memcpy(str_temp,buff,bytes);
	str_temp[bytes] = '\0';
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
        clock_t t;                //to measure CPU  time
	struct timeval start,end; //to measure actual execution time
	
        double time_diff;//to measure the timer for the execution time of code
	int rand_num;
	unsigned short int check;
	void* buff;
	char buf[65535]={0, };
	char new_buff[65535]={0, };  
	char ack_buf[65535]={0, };
	char* temp;
	char checksum_info[1024]={0, } ;
	char bytes_info[1024] = {0, } ;
	unsigned short int bytes_len;
	char bytes_len_str[1024]={0, };
	char prev = '1';
	char seq_no;
	char current;
	char data_1[75535]={0, };
	unsigned  short int check_length;
	char check_len_str[1024]={0, };
	char ack_check_len[1024]={0, };
        unsigned short int  ack_checksum_len; 	
	char ack_check[65535]={0, };
	unsigned short int ack_checksum;
        char temp_buf[65535]={0, };
        unsigned short int actual_checksum;
        unsigned short int length1;
	char content[65535]={0, };
        int i,j;
	int total_len;
	char *data = NULL;
	char *tmp = NULL;
        char* temp_ack_rcv;
	char* ack_rcv = NULL;


	t = clock();
	gettimeofday(&start,NULL);
	data = calloc(1, 65536);
	if (!data)
		return -1;

	tmp = data;
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

	
	ack_rcv = calloc(1,65535);   //allocating memory for the ack received after rand_corrupt
	if(ack_rcv == NULL)
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
	FILE *fp = fopen("home.jpg","rb");
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
	 printf("packets =%s\n",(char*)buff);

	n= sendto(sock,buff,1024,0,(struct sockaddr *)&server,sizeof(struct sockaddr));
        if(n<0)
        {
	       	printf("error in sending message to the server");
		return 1;
	}
	


	int loop =1;
	/*Read data from file and send it*/
	int packetNum = 0;

	
	while(1)
	{


	        memset (buf, 0, 65535);      //clear the contents of buf before fread
		memset(checksum_info,0,1024);
		memset(check_len_str,0,1024);
                memset(bytes_info,0,1024);
		memset(bytes_len_str,0,1024);
	        memset(data,0,65535);	
				       
		printf("iteration %d\n", loop);
		/*First read file in chunks of  1024  bytes */

		int nread = fread(buf,1,1024,fp);
		printf("Bytes read %d\n",nread);
            
		/*if read was success ,send data*/
		if(nread>0)
		{


			 
		         check = checksum(buf,nread);    //calculate the checksum
			 printf("checksum is %d\n",check); 
    			 itoa(check,checksum_info,10);  //convert checksum into string
			 check_length = strlen(checksum_info);  //calculate the length of the checksum
			 itoa(check_length,check_len_str,10); //convert the checksum lenght to string form
			 

			 itoa(nread,bytes_info,10);    //the data length in string format
			 bytes_len = strlen(bytes_info); 
                         itoa(bytes_len,bytes_len_str,10);
		
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

			
	         
	 				
                        data[0] = seq_no;     //combine seq no,checksum and the data content into one packet 
                        printf("sequence number is %c\n",data[0]);
                        
  		

			strncat(data,check_len_str,(strlen(check_len_str)));
			
			strncat(data,checksum_info,(strlen(checksum_info))); //adding checksum
		

			strncat(data,bytes_len_str,strlen(bytes_len_str)); //adding data length
			

			strncat(data,bytes_info,strlen(bytes_info)); //adding the data length info


	               
			tmp = data + 3 + bytes_len + check_length; //memcpy has to be used because strcat or strncat will terminate once they encounter a null character
			memcpy (tmp, buf, nread);

                 
			
                        total_len = nread+3+bytes_len+check_length;//finding the total length
			data[total_len] = '\0';
       			
			n= sendto(sock,data,total_len,0,(struct sockaddr *)&server,sizeof(struct sockaddr));///send the data packet after adding the header info
			
			packetNum++; //increment the packets sent
	                if(n<0)
                 	{
		              printf("error in sending message to the server");
			      fclose(fp);
			      return 1;
			}
                        
			int msec = 0, trigger = 50; /*50 ms*/
			clock_t initial = clock();  /*start the timer*/

                        
		        while(1)      //wait for the acknowledgement for the sent packet till timeout , here timer value is set to 50 ms
			{
			   clock_t difference = clock()-initial;
			   msec = difference*1000/CLOCKS_PER_SEC;
                          
			  


                           if(msec<trigger)
		           {
		              memset(ack_buf, 0, 65535);        //clear the contents of ack_buf
			      memset(ack_check_len,0,1024);
			      memset(ack_check,0,65535);
			      memset(content,0,65535);
			      memset(temp_buf,0,65535);
			   
			      n =  recvfrom(sock,ack_buf,1024,MSG_DONTWAIT,&server, &length); //receive the ack from the server
			      if(n>0)
		             {
		               rand_num = 100;  //let there be no loss of ACK
 
		                if(rand_num <5)//loss occured
		                {

				    //do nothing , ignore the ACK,to simulate loss
		                }
			       else  //consider the ACK received
		               {  
			         seq_no = ack_buf[0];  //first byte will  be sequence number
			         printf("Ack received for the sequence number %c\n",seq_no);
         		         if(seq_no == current)    //if the sequence number matches then proceed 
		                 {

				      printf("Ack received\n");

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
		               	       while(i<n)
			           {
				   content[j] = ack_buf[i];
				   i++;
				   j++;
		                    }
				       content[j] ='\0';
				      length1 = strlen(content);

                                          
                                  temp_ack_rcv = rand_corrupt(content,length1); //corrupt the ACK to simulate ACK error
				  memcpy(ack_rcv,temp_ack_rcv,length1);
                                  ack_rcv[length1] = '\0';
                                  actual_checksum =  checksum(ack_rcv,length1);//calculate the actual checksum
			

				  printf("checksum of the ack received is %d\t and the checksum of the ack sent                                           is %d\n",actual_checksum,ack_checksum);
			          if(actual_checksum == ack_checksum) //if the ack is not corrupt then break the loop and send the next packet
			          {
				      printf("success go to next packet\n");
		                       break;
				  }
			          else  //if the ACK is corrupt wait till time out
		                  {

	                            printf("ACK is corrupt, wait till timeout\n");                 
				  }
				}
			    else  //even if the sequence number of the ack packet does not match then wait till timeout
		           { 
				printf("Sequence number of ACK does not macth,wait till timeout\n");

                                                       	     
                           }
			  }

			 }
			
                        }
		       
			else //if the timer is out and ACK didnot arrive then resend the packet
		 
                       {
		
		   	  printf("timeout and ACK has not been received yet\n");
			  printf("data has been sent again\n");
                          n = sendto(sock,data,total_len,0,(struct sockaddr *)&server,sizeof(struct sockaddr));	
			  initial = clock();  //start the timer again
		       }

		     }
                              
			
		   
                    	if(current == '0')
				prev = '0';
			else
				prev = '1';
			
			


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

                loop++;

	}

	strcpy(buff,"Finish");
	n= sendto(sock,buff,1024,0,(struct sockaddr *)&server,sizeof(struct sockaddr));
        if(n<0)
        {
	       	printf("error in sending message to the server");
		return 1;
	}
	
	
	fclose(fp); //close the  file to complete the transmission
	t = clock()-t;
	gettimeofday(&end,NULL);

	double time_taken =  ((double)t)/CLOCKS_PER_SEC; //in secconds
	printf(" The CPU time for transmission  %f seconds \n",time_taken);

	double delta = ((end.tv_sec - start.tv_sec)*1000000u+ end.tv_usec - start.tv_usec)/1.e6;
	printf("The actual execution time took %f seconds\n", delta);
	close(sock); //close api tries to complete the transmission if there is data waiting to  be transmitted

	return 0;
} 
