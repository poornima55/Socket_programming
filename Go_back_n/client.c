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

void rand_wait()
{
	int num;
	int i;
	num = rand() %100;

	if(num<5)
	{
              for(i=0;i<10;i++)
	      {
			sleep(1);
	       
	      }
	}
	return;
}


int 
main (int argc, char **argv)
{
	int sock,length,n;
	struct sockaddr_in server,from;
	struct hostent *hp;
	long int packets =0;
        clock_t t;                //to measure CPU  time
	struct timeval start,end; //to measure actual execution time
	struct timeval timer; //for the timer implementation
        double time_diff;//to measure the timer for the execution time of code
	unsigned short int check;
	void* buff;
	printf("initializing 2 d array\n");

	char buf[1024][1024]={0};
	printf("initialisation failed\n");
	int nread[1024] = {0,};
	char new_buff[65535]={0, };  
	char ack_buf[65535]={0, };
	char* temp;
	char checksum_info[1024]={0, } ;
	char bytes_info[1024] = {0, } ;
	unsigned short int bytes_len;
	char bytes_len_str[1024]={0, };
        char seqnum[10] = {0,};
	int seq_no;
	char seq_no_char[10] ={0,};
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
	char end_flag =0;  //to mark the end of file or error occured while reading file
	int total_len[1024]={0,};
	char data[1024][6553]= {0,};
	char *tmp = NULL;
        int msec,trigger;
	clock_t difference,initial;
	t = clock();

	gettimeofday(&start,NULL);

	
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
	FILE *fp = fopen("Lab2.txt","rb");
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

	int N = 2;
	int base = 1;
	int nextseqnum =1;
	int sequence;
        int seqnum_next = 1;
	while(1)
	{


	     while(nextseqnum<base+N)   //send N consecutive packets
	     {

                
	        memset(buf[nextseqnum], 0, 65535);      //clear the contents of buf before fread
		memset(checksum_info,0,1024);
		memset(check_len_str,0,1024);
                memset(bytes_info,0,1024);
		memset(bytes_len_str,0,1024);
	        memset(data[nextseqnum],0,6553);	
				       
	
		/*First read file in chunks of  1024  bytes */

	        nread[nextseqnum] = fread(buf[nextseqnum],1,1024,fp);
		printf("Bytes read %d\n",nread[nextseqnum]);
             
		/*if read was success ,send data*/
		if(nread[nextseqnum]>0)
		{
		     
			 		check = checksum(buf[nextseqnum],nread[nextseqnum]);    //calculate the checksum
				        printf("checksum is %d\n",check); 
    			 		itoa(check,checksum_info,10);  //convert checksum into string
			 		check_length = strlen(checksum_info);  //calculate the length of the checksum
			 		itoa(check_length,check_len_str,10); //convert the checksum lenght to string form

			 		itoa(nread[nextseqnum],bytes_info,10);    //the data length in string format
			 		bytes_len = strlen(bytes_info); 

                         		itoa(bytes_len,bytes_len_str,10);
	         
                       		        itoa(seqnum_next,seqnum,10);     //combine seq no,checksum and the data content into one packet 
                        		printf("sequence number is %s\n",seqnum);
		                        
		                        strncat(data[nextseqnum],seqnum,strlen(seqnum));
                                        printf("sequence number sent in the packet is %c\n",data[nextseqnum][0]);

  					strncat(data[nextseqnum],check_len_str,(strlen(check_len_str)));
			
					strncat(data[nextseqnum],checksum_info,(strlen(checksum_info))); //adding checksum
					strncat(data[nextseqnum],bytes_len_str,strlen(bytes_len_str)); //adding data length
			
					strncat(data[nextseqnum],bytes_info,strlen(bytes_info)); //adding the data length info		
					j= 3+bytes_len+check_length;
                        		for(i=0;i<nread[nextseqnum];i++)
					{
						data[nextseqnum][j] = buf[nextseqnum][i];
					        j++;
					}


					total_len[nextseqnum] = nread[nextseqnum]+3+bytes_len+check_length;//finding the total length
					data[nextseqnum][total_len[nextseqnum]] = '\0';
       			
					n= sendto(sock,data[nextseqnum],total_len[nextseqnum],0,(struct sockaddr *)&server,sizeof(struct sockaddr));///send the data packet after adding the header info
			
		
	                		if(n<0)
                 			{
		              			printf("error in sending message to the server");
			      			fclose(fp);
			      			return 1;
					}
                       			 if(nextseqnum == base)
					{
						 msec = 0;
						 trigger = 50; /*50 ms*/
					         initial = clock();  /*start the timer*/
                       			 }

                                        nextseqnum++;
					seqnum_next = nextseqnum % N;
					

			   }
				
			
			
		        
	  	  /*There is something tricky going on with the read..
		   * Either there was error ,or we reached end of  file.
		  */
		  else 
		  {
			if(feof(fp))
				printf("End of file\n");
				
			if(ferror(fp))
				printf("Error reading\n");
			end_flag = 1;            //set the flag that it has reached EOF or encountered error
			break;
		  }

		}



                     

	        while(1)      //wait for the acknowledgement for the sent packets till timeout , here timer value is set to 50 ms
		{
			    difference = clock()-initial;
			    msec = difference*1000/CLOCKS_PER_SEC;
                             
                          if(msec<trigger) //if it is not timeout then receive the ack packets
		          {
		                 memset(ack_buf, 0, 65535);        //clear the contents of ack_buf
			         memset(ack_check_len,0,1024);
			   	 memset(ack_check,0,65535);
			    	 memset(content,0,65535);
			    	 memset(temp_buf,0,65535);
			   
			    	 n =  recvfrom(sock,ack_buf,1024,MSG_DONTWAIT,&server, &length); //receive the ack from the server
			    	if(n>0)
		            	{
                                 
					j=0;
					for(i=0;i<1;i++)
					{
			    		       seq_no_char[j]= ack_buf[i]; 
					       j++;	//first 2 bytes will  be sequence number
					}

					seq_no = atoi(seq_no_char);
			    			printf("Ack received for the sequence number %d\n",seq_no);
         		    
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
                            		actual_checksum =  checksum(content,length1);//calculate the actual checksum
		            		printf("checksum of the ack received is %d\t and the checksum of the ack sent                                           is %d\n",actual_checksum,ack_checksum);
			    	
					
					if(actual_checksum == ack_checksum) //if the ack is not corrupt then update the base
			    		{
				      		printf("successfully recived packet %d\n",seq_no);
				     		base =seq_no+1;
				      		if(base == seqnum_next)  //go to sending next series of packets
			              		{
					   		break;
				      		 }
				      		else                              
			              		{                                
						      initial = clock(); //start timer for next in-flight packet
			              		}


		                     
		                         }
			     
			                else  //if it is a corrupt ack
			               {


                                          printf("Ack corrupted,wait till timeout\n");
			     
                                        }
			  
			           }

		              }	

			  

		       
			else //if the timer is out, resend the packet from the packet with seqno 
			       //	base till nextseqnum-1,start timer
		 
                       {

			  initial = clock();

			  sequence =  base;
                          while(sequence <= nextseqnum-1)
		          {
					
					n= sendto(sock,data[sequence],total_len[sequence],0,(struct sockaddr *)&server,sizeof(struct sockaddr));///send the data packet after adding the header info
			
		
	                		if(n<0)
                 			{
		              			printf("error in sending message to the server");
			      			fclose(fp);
			      			return 1;
					}
                       		


			  
		                 }
                              
			
			     }

                    


	         	}
		
                if(end_flag == 1)  //if there had been an error in reading file of EOF had reached break out of 
			           // the outermost while loop
		{
			break;
		}
               

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
