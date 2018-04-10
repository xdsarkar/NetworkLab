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
#define MAX_RETRY 5 /* MAX retries in case of failure*/
#define TIME_OUT 10 /* In seconds */

/* TFTP RFC 1350 PACKING */
static const char* MODE="octet";
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERR 5
#define strMODE strlen(MODE)

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define PROMPT ANSI_COLOR_RED "tftp> " ANSI_COLOR_RESET

void error(char *s) { perror(s); exit(1); }

void fill_struct(char* server_hostname, struct sockaddr_in *server_address, int addrlen)
{
    memset((char *)server_address, 0, addrlen);
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(PORT);
    if(inet_aton(server_hostname , &(server_address->sin_addr)) == 0) exit(1); 
}

struct sockaddr_in server_address, reply_address;
struct timeval timeout;
int sockfd, slen;

void help()
{
	printf("usage: <get <file: name>>\n");
	printf("usage: <put <file: name>>\n");
	printf("usage: <server> -> (server address)\n");
	printf("usage: <change <server address>> -> (change server address)\n");
	printf("usage: <help> or <tftp --h> -> (help)\n");
	printf("usage: <exit> -> (exit from tftp client)\n");
}

void send_file(char *hostname ,char *filename)
{
	char message[MSGLEN], buf[BUFLEN];
	int filename_len, datagram_len, blocknum, addrlen, ackblock;
	
	blocknum=0;
    addrlen = sizeof(reply_address);

	FILE *fp = fopen(filename, "r");
	if(fp==NULL) error("error: fopen() failure");

	memset(message, '\0', MSGLEN);
	message[0]=0x0;
	message[1]=WRQ;
	strcpy(message+2, filename);
	filename_len = strlen(filename);
	strcpy(message+2+filename_len+1, MODE);
	datagram_len=2+filename_len+1+strMODE+1;

	if (sendto(sockfd, message, datagram_len, 0, (struct sockaddr *)&server_address, slen)==-1) error("sendto() failure");
    printf("success: Sent [WRQ]\n");

    int i; while(1)
    {
    	for(i=1; i<=MAX_RETRY; i++)
    	{
    		memset(buf, '\0', BUFLEN);
		    if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *)&reply_address, &addrlen) == -1) error("recvfrom() failure");
		    ackblock = (buf[2]<<8) + buf[3];
		    if((buf[1] == ERR) || (ackblock == (blocknum-1)))
		    {
		    	printf("Error sending block %d. Reattempted.\n", blocknum);
		    	if (sendto(sockfd, message, BUFLEN, 0, (struct sockaddr *)&server_address, slen)==-1) error("sendto() failure");
		    }
		    else break;
    	}
    	if(i>MAX_RETRY)
		{
			printf("error: Max retries failed. Exiting \n" );
			return;
		}
    	
	    printf("[ACK] : BLOCK [%d] recieved\n", blocknum);

	    blocknum++;
    	memset(message, '\0', MSGLEN);
    	message[1]=DATA;
		message[2]= 0x0; //(blocknum & 0xff)<<8;
		message[3]=(blocknum & 0xff);
		// message[2]=blocknum/256;
		// message[3]=blocknum%256;

    	int n = fread(message+4 , 1 , 512 , fp);
    	printf("Sending block %d of %d bytes.\n", blocknum,n);
    	if (sendto(sockfd, message, n+4 , 0 , (struct sockaddr *) &reply_address, addrlen)==-1) error("error: sendto() failure");
    	if(n<512) break;
    }
    printf(">>> Transfer complete --> [%s] <<<\n", hostname);
    fclose(fp);
}

void recv_file(char *hostname ,char *filename)
{
	char message[MSGLEN], buf[BUFLEN];
	int filename_len, blocknum, addrlen;
	FILE *fp = fopen(filename, "w");

	memset(message, '\0', MSGLEN);
	message[0]=0x0;
	message[1]=RRQ;
	strcpy(message+2, filename);
	filename_len = strlen(filename);
	strcpy(message+2+filename_len+1 , MODE);

	if(sendto(sockfd, message, MSGLEN, 0, (struct sockaddr *)&server_address, slen)==-1) error("error: sendto() failure");
    printf("Sent [RRQ]\n");

    blocknum=1;
    addrlen= sizeof(reply_address);

    while(1)
    {
    	addrlen= sizeof(reply_address);
    	bzero(buf,516);
    	int n = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *)&reply_address, &addrlen);
    	if (n == -1) error("error: recvfrom() failure");

	    if(buf[1] == ERR) error("error: get failure");
	    fwrite(&buf[4], 1, n-4, fp);
	    printf("success: Received block of size = [%d] bytes\n", n-4);

	    bzero(message, BUFLEN);
		message[0]=0x0;
		message[1]=ACK;
		message[2]=0x0; //(blocknum & 0xff)<<8;
		message[3]=(blocknum & 0xff);
		// message[2]=blocknum/256;
		// message[3]=blocknum%256;
		if(sendto(sockfd, message, 4, 0, (struct sockaddr *)&reply_address, addrlen)==-1) error("error: sendto() failure");  	
    	printf("Sent [ACK] for Block [%d]\n", blocknum);
		blocknum++;
	    if(n<512) break;
    }
    printf(">>> Transfer complete --> [%s] <<<\n", hostname);
    fclose(fp);
}
int main(int argc, char **argv)
{
	printf("\033[H\033[J");
	char *server_hostname;
	char file[412], operation[100], command[512];
	if(argc == 1) server_hostname = "127.0.0.1";
    else server_hostname = argv[1];
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) error("error: socket() failure\n");
    slen = sizeof(server_address);
    
    /* Timeout 10 seconds */
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) error("error: setsockopt failed\n");
    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) error("error: setsockopt failed\n");

	while(1)
	{
		fill_struct(server_hostname, &server_address, slen);
		printf(PROMPT);
		fgets(command, 512, stdin);
		sscanf(command,"%s %s", operation, file);
		if(strcmp(operation,"exit")==0) break;
		else if(strcmp(operation,"get")==0 && strcmp(file,"")!=0) recv_file(server_hostname, file);
		else if(strcmp(operation,"get")==0 && strcmp(file,"")==0) printf("usage: get <file:name>\n");
		else if(strcmp(operation,"put")==0 && strcmp(file,"")!=0) send_file(server_hostname, file);
		else if(strcmp(operation,"put")==0 && strcmp(file,"")==0) printf("usage: put <file:name>\n");
		else if(strcmp(operation,"help")==0 || strcmp(file,"tftp --h")==0) help();
		else if(strcmp(operation,"server")==0) printf("TFTP Server -> [%s:%d]\n", server_hostname, PORT);
		else if(strcmp(operation,"change")==0 && strcmp(file,"")!=0) strncpy(server_hostname, file, strlen(file));
		else if(strcmp(operation,"change")==0 && strcmp(file,"")==0) printf("usage: change <server:ip>\n"); 
		else printf("Invalid command\n");
	}
	close(sockfd);
	return 0;
}