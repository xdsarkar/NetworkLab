#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>

#define BUFLEN 516 /* Maximum buffer length */
#define MSGLEN 516 /* Maximum message length */
#define PORT 6900 /* TFTP server hosted PORT */
#define RETRY 5 /* MAX retries in case of failure*/
#define TIME_OUT 5 /* In seconds */

const char* MODE="octet";
#define strMODE strlen(MODE)

/* Opcodes */
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERR 5

/* Color RED */
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define PROMPT ANSI_COLOR_RED "tftp> " ANSI_COLOR_RESET

struct sockaddr_in server_address, reply_address;
struct timeval timeout;
int sockfd, server_addrlen;

void error(char *s) { perror(s); exit(1); }

void help();
void recv_file(char *, char *);
void send_file(char *, char *);
int check_timeout(int);

void fill_structure(char* server_hostname, struct sockaddr_in *server_address, int addrlen)
{
    memset((char *)server_address, 0, addrlen);
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(PORT);
    if(inet_aton(server_hostname , &(server_address->sin_addr)) == 0) exit(1); 
}

int main(int argc, char **argv)
{
	printf("\033[H\033[J"); /* clears screen */
	char *server_hostname;
	char file[412], operation[100], command[512];

	if(argc < 2) server_hostname = "127.0.0.1";
    else server_hostname = argv[1];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) error("error: socket() failure\n");
    server_addrlen = sizeof(server_address);
    
    /* Timeout 10 seconds */
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) error("error: setsockopt failed\n");
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) error("error: setsockopt failed\n");

	while(1)
	{
		fill_structure(server_hostname, &server_address, server_addrlen);
		printf(PROMPT);
		fgets(command, 512, stdin);
		sscanf(command,"%s %s", operation, file);
		if(strcmp(operation,"exit")==0) break;
		else if(strcmp(operation,"get")==0 && strcmp(file,"")!=0) recv_file(server_hostname, file);
		else if(strcmp(operation,"get")==0 && strcmp(file,"")==0) printf("usage: get <file:name>\n");
		else if(strcmp(operation,"put")==0 && strcmp(file,"")!=0) send_file(server_hostname, file);
		else if(strcmp(operation,"put")==0 && strcmp(file,"")==0) printf("usage: put <file:name>\n");
		else if(strcmp(operation,"help")==0) help();
		else if(strcmp(operation,"server")==0) printf("TFTP Server -> [%s:%d]\n", server_hostname, PORT);
		else if(strcmp(operation,"change")==0 && strcmp(file,"")!=0) strncpy(server_hostname, file, strlen(file));
		else if(strcmp(operation,"change")==0 && strcmp(file,"")==0) printf("usage: change <server:ip>\n");
		else if(strcmp(operation,"ls")==0 && strcmp(file, "client")==0)
		{
			/* Limited to local machine */
			char *args[2] = {"ls", NULL};
			pid_t pid;
			if ((pid = fork()) == 0) execvp(args[0], args);
			else if(pid > 0) wait(NULL);
		}
		else printf("Invalid command\n");
		/*Clear buffers */
		memset(command, '\0', 512);
		memset(operation, '\0', 100);
		memset(file, '\0', 412);
	}
	close(sockfd);
	return 0;
}

void send_file(char *hostname, char *filename)
{
	char message[MSGLEN], buf[BUFLEN];
	int filename_len, datagram_len, blocknum, addrlen, ack_block, numbytes;

	FILE *fp = fopen(filename, "r");
	if(fp==NULL) /* Wrong File Name/ Parse Error */ 
	{
		printf("error: fopen() failure. Try again?\n");
		return;
	}

	memset(message,'\0',MSGLEN);
	/* WRQ PACK */
	message[0]=0x0;
	message[1]=WRQ;
	strcpy(message+2, filename);
	filename_len = strlen(filename);
	strcpy(message+2+filename_len+1, MODE);
	/* PACK DONE */
	datagram_len=2+filename_len+1+strMODE+1;

	if (sendto(sockfd, message, datagram_len, 0, (struct sockaddr *)&server_address, server_addrlen)==-1) error("sendto() failure");
    printf("success: Sent [WRQ]\n");

    blocknum=0;
    addrlen = sizeof(reply_address);

    while(1)
    {
	    int times; for(times=0;times<=RETRY;++times)
		{
			if(times == RETRY)
			{
				// reached max no. of tries
				printf("tftp: Max tries reached. Exiting.\n");
				exit(1);
			}
			memset(buf, '\0', BUFLEN);
			recvfrom(sockfd, buf, BUFLEN , 0, (struct sockaddr *)&reply_address, &addrlen);
			// checking if timeout has occurred or not
			ack_block = (buf[2]<<8)+buf[3];
		    /* Stop and Wait */
		    if((buf[1] == ERR) || (ack_block == (blocknum-1)))
		    {
		    	printf("Error sending block %d. Reattempted.\n", blocknum);
		    	if(sendto(sockfd, message, BUFLEN, 0, (struct sockaddr *)&server_address, server_addrlen)==-1) error("sendto() failure");
		    }
		    else break;

			numbytes = check_timeout(sockfd);
			if(numbytes == -1)
			{
				//error
				perror("tftp: recvfrom");
				exit(1);
			} 
			else if(numbytes == -2)
			{
				//timeout
				printf("tftp: Reattempts left = %d\n", times+1);
				int temp_bytes;
				if((temp_bytes = sendto(sockfd, message, BUFLEN, 0, (struct sockaddr *)&server_address, server_addrlen))==-1)
				{
					perror("tftp: ACK (sendto)");
					exit(1);
				}
				printf("tftp: sent %d bytes [packet]\n", temp_bytes);
				continue;
			} 
			else break;
		}

		printf("[ACK] : BLOCK [%d] recieved\n", blocknum);
	    blocknum++;
    	memset(message, '\0', MSGLEN);
    	/* DATA PACK */
    	message[1]=DATA;
		message[2]=blocknum>>8;
		message[3]=blocknum%(0xff+1);
		// message[2]=blocknum/256;
		// message[3]=blocknum%256;
    	int n = fread(message+4, 1, 512, fp);
    	printf("Sending block [%d] of [%d] bytes\n", blocknum, n);
    	if (sendto(sockfd, message, n+4 , 0 , (struct sockaddr *)&reply_address, addrlen)==-1) error("error: sendto() failure");
    	if(n<512) break;
	}

    printf(">>> Transfer complete --> [%s] <<<\n", hostname);
    fclose(fp);
    return;
}

void recv_file(char *hostname ,char *filename)
{
	FILE *fp;
	char message[MSGLEN], buf[BUFLEN];
	int filename_len, blocknum, addrlen, count=0;
	memset(message, '\0', MSGLEN);
	/* RRQ PACK */
	message[0]=0x0;
	message[1]=RRQ;
	strcpy(message+2, filename);
	filename_len = strlen(filename);
	strcpy(message+2+filename_len+1 , MODE);
	/* PACK DONE */

	if(sendto(sockfd, message, MSGLEN, 0, (struct sockaddr *)&server_address, server_addrlen)==-1) error("error: sendto() failure");
    printf("Sent [RRQ]\n");
    blocknum=1;
    while(1)
    {
    	addrlen = sizeof(reply_address);
    	memset(buf, '\0', BUFLEN);
    	int n = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *)&reply_address, &addrlen);
    	if (n == -1)
    	{
    		error("error: recvfrom() failure");
    		return;
    	}
    	if(buf[1]==ERR && count == 0)
    	{
    		printf("error: File not present/ Error\n");
    		return;
    	}
    	else if(buf[1]!=ERR && count == 0) fp = fopen(filename, "w");
    	++count;

	    fwrite(&buf[4], 1, n-4, fp);
	    printf("success: Received block of size = [%d] bytes\n", n-4);

	    memset(message, '\0', MSGLEN);
	    /* ACK PACK */
		message[0]=0x0;
		message[1]=ACK;
		message[2]=blocknum>>8;
		message[3]=blocknum%(0xff+1);
		// message[2]=blocknum/256;
		// message[3]=blocknum%256;
		/* PACK DONE */
		if(sendto(sockfd, message, 4, 0, (struct sockaddr *)&reply_address, addrlen)==-1) error("error: sendto() failure");  	
    	printf("Sent [ACK] for Block [%d]\n", blocknum);
		blocknum++;
	    if(n<512) break;
    }
    printf(">>> Transfer complete --> [%s] <<<\n", hostname);
    fclose(fp);
    return;
}

int check_timeout(int sockfd)
{
	fd_set fds;
	int n;
	struct timeval tv;

	// set up the file descriptor set
	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);

	// set up the struct timeval for the timeout
	tv.tv_sec = TIME_OUT;
	tv.tv_usec = 0;

	// wait until timeout or data received
	n = select(sockfd+1, &fds, NULL, NULL, &tv);
	if (n == 0)
	{
		printf("tftp: Timeout\n");
		return -2; // timeout!
	}
	else if (n == -1)
	{
		printf("tftp: Error\n");
		return -1; // error	
	}
	else return 0;
}

void help()
{
	printf(ANSI_COLOR_RED"-----------------------------------------------------------\n");
	printf("usage: <get <file: name>>\n");
	printf("usage: <put <file: name>>\n");
	printf("usage: <server> -> (server address)\n");
	printf("usage: <change <server address>> -> (change server address)\n");
	printf("usage: <help> or <tftp --h> -> (help)\n");
	printf("usage: <exit> -> (exit from tftp client)\n");
	printf("usage: <ls> <client> -> (list files in the file directory)\n");
	printf("-----------------------------------------------------------\n"ANSI_COLOR_RESET);
}