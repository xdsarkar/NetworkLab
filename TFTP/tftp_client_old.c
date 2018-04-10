#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFLEN 516 /* Maximum buffer length */
#define MSGLEN 516 /* Maximum message length */
#define PORT 6900 /* TFTP server hosted PORT */
#define MAXRETR 5 /* MAX retries in case of failure*/
#define TIME_OUT 10 /* In seconds */

/* TFTP RFC 1350 PACKING */
static const char* MODE="octet";
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERR 5
#define strMODE strlen(MODE)
#define port 6900

void error(char *s) { perror(s); exit(1); }

void fill_struct(char* server_hostname, struct sockaddr_in *serv_addr, int addrlen)
{
    memset((char *)serv_addr, 0, addrlen);
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(port);
     
    if (inet_aton(server_hostname , &(serv_addr->sin_addr)) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }    
}

struct sockaddr_in serv_addr, reply_addr;
int sockfd;

void help()
{
	printf("The available commands are as follows. They all have their usual meainings.\n");
	printf("put <filename>\n");
	printf("get <filename>\n");
	printf("quit\n");
	printf("help\n");
}
void send_file(char *hostname ,char *filename)
{
	int filename_len, req_len, blocknum, addrlen, ackblock, slen;
	char message[516], buf[516];
	FILE *fp = fopen(filename, "r");
	if(fp==NULL) error("Error opening file.");
	
	bzero(message, 516);
	message[0]=0x0;
	message[1]=WRQ;
	strcpy(message+2, filename);
	filename_len = strlen(filename);
	strcpy(message+2+filename_len+1, MODE);
	req_len=2+filename_len+1+strMODE+1;

	if (sendto(sockfd, message, req_len , 0 , (struct sockaddr *) &serv_addr, slen)==-1) error("sendto()");
    printf("Sent WRQ.\n");
  

    int n;
    blocknum=0;
    addrlen = sizeof(reply_addr);

    int i; while(1)
    {
    	for(i=1; i<=MAXRETR; i++)
    	{
    		bzero(buf,BUFLEN);
		    if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &reply_addr, &addrlen) == -1)
		    {
		        error("recvfrom()");
		    }
		    ackblock = (buf[2]<<8) + buf[3];

		    if((buf[1]==ERR) || (ackblock==(blocknum-1)))
		    {
		    	printf("Error sending blocknum, trying  again.%d\n", blocknum);
		    	if (sendto(sockfd, message, BUFLEN , 0 , (struct sockaddr *) &serv_addr, slen)==-1) error("sendto()");
		    }
		    else break;
    	}
    	if(i>MAXRETR)
		{
			printf("Giving up on sending file. :( \n" );
			return ;
		}
    	
	    printf("ACK received for block number %d.\n", blocknum);

	    blocknum++;
    	bzero(message, BUFLEN);
    	message[1]=DATA;
		message[2]= 0x0; //(blocknum & 0xff)<<8;
		message[3]=(blocknum & 0xff);
		// message[2]=blocknum/256;
		// message[3]=blocknum%256;

    	int n = fread(message+4 , 1 , 512 , fp);
    	printf("Sending block %d of %d bytes.\n", blocknum,n);
    	if (sendto(sockfd, message, n+4 , 0 , (struct sockaddr *) &reply_addr, addrlen)==-1) error("sendto()");
    	if(n<512) break;
    }
    fclose(fp);
    printf("Transfer complete.\n");
}

void recv_file(char *hostname ,char *filename)
{
	int filename_len, slen, blocknum, addrlen;
	FILE *fp = fopen(filename, "w");
	filename_len = strlen(filename);

	char message[516]  , buf[516];
	bzero(message, BUFLEN);
	message[0]=0x0;
	message[1]=RRQ;
	strcpy(message+2, filename);
	strcpy(message+2+filename_len+1 , MODE);

	if(sendto(sockfd, message, 516 , 0 , (struct sockaddr *) &serv_addr, slen)==-1) error("sendto()");
    printf("Sent RRQ.\n");

    int n;
    blocknum=1;
    addrlen= sizeof(reply_addr);

    while(1)
    {
    	addrlen= sizeof(reply_addr);
    	bzero(buf,516);
    	n = recvfrom(sockfd, buf, 516, 0, (struct sockaddr *) &reply_addr, &addrlen);
    	
    	if (n == -1) error("recvfrom()");
	    if(buf[1]==ERR) error("Server transfer failure");

	    fwrite(&buf[4],1,n-4,fp);
	    printf("Received block of size n = %d\n", n-4);

	    bzero(message, BUFLEN);
		message[0]=0x0;
		message[1]=ACK;
		message[2]=0x0; //(blocknum & 0xff)<<8;
		message[3]=(blocknum & 0xff);
		// message[2]=blocknum/256;
		// message[3]=blocknum%256;
		if (sendto(sockfd, message, 4 , 0 , (struct sockaddr *) &reply_addr, addrlen)==-1) error("sendto()");  	
    	printf("Sent ACK for block %d.\n", blocknum);
		blocknum++;
	    if(n<512) break;
    }
    fclose(fp);
    printf("Transfer complete.\n");

}
int main(int argc , char **argv)
{
	char *server_hostname;
	char file[410], operation[100], command[512];
	if(argc == 1) server_hostname = "127.0.0.1";
    else server_hostname = argv[1];
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) error("socket() failure\n");
    fill_struct(server_hostname, &serv_addr, sizeof(serv_addr));
    
    struct timeval timeout;      
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) error("setsockopt failed\n");
    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) error("setsockopt failed\n");

	while(1)
	{
		printf("tftp>");
		fgets(command,512,stdin);
		sscanf(command,"%s %s",operation,file);
		if(strcmp(operation,"exit")==0) break;
		else if(strcmp(operation,"get")==0 && strcmp(file,"")!=0) recv_file(server_hostname, file);
		else if(strcmp(operation,"get")==0 && strcmp(file,"")==0) printf("usage: get <file:name>\n");
		else if(strcmp(operation,"put")==0 && strcmp(file,"")!=0) send_file(server_hostname, file);
		else if(strcmp(operation,"put")==0 && strcmp(file,"")==0) printf("usage: put <file:name>\n");
		else if(strcmp(operation,"help")==0 || strcmp(file,"tftp --h")==0) help();
	}
	close(sockfd);
	return 0;
}