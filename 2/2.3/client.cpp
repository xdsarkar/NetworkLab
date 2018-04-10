#include<stdio.h> //Input/ Output handler
#include<string.h>    //strlen, memset, bzero
#include<sys/types.h> //This header file contains definitions of a number of data types used in system calls. These types are used in the next two include files.
#include<sys/socket.h> //The header file socket.h includes a number of definitions of structures needed for sockets.
#include<arpa/inet.h> //inet_addr The header file netinet/in.h contains constants and structures needed for internet domain addresses.
#include<unistd.h>    //write
#include<stdlib.h>
#include<iostream>
#include<pthread.h>

using namespace std;

#define prePO 8000 //user defined free port
#define max_len 4500

char client_message[max_len]; //client_msg buffer
char other_client_message[max_len];

void send_func(int sock) //normal void function
{
    int n;
    //char *buff = (char *)malloc(max_len);
    char buff[max_len];
    while(1)
    {
        scanf("%s",buff);
        if((strncmp(buff,"end",3)==0))
        {
            printf("Bye!\n");
            close(sock);
            exit(0);
        }
        else
        {
            send(sock,buff,strlen(buff)+1,0);
            buff[0]='\0';
        }
    }
}

void *rece (void *sock) //thread read
{
    int n;
    char buff[max_len];
    int socket_desc = *(intptr_t *)sock;
 
    while(1)
    {
        n =recv(socket_desc,&buff,sizeof(buff),0);
        if(n<=0)
        {
            close(socket_desc);
            exit(0);
        }
        buff[n]='\0';
        printf("%s\n",buff);
        buff[0]='\0';
    }
}

int main(int argc , char *argv[])
{
    int PORT;
    int sock;
    char name[100];
    char message[100];
    if(argc<2) PORT=prePO;
    else PORT=atoi(argv[1]);

    int numbytes;
    struct sockaddr_in server;
     
    //Create socket
    sock = socket(PF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf(">Error: Socket creation failed\n");
    }
    else printf(">Socket created\n");
     
    //structure of sockaddr_in comprises of the following fields
    server.sin_addr.s_addr = INADDR_ANY; //inet_addr("127.0.0.1") //IPV4 address (4 byte)
    server.sin_family = AF_INET; //IPv4
    server.sin_port = htons(PORT); //host to network, changes byte order
    
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //errno = 0: program starts, errno changes according to categories of error, function or program
        perror(">Error: Connect failed"); //last error number, i.e., errno; perror should be called right after the error was produced
        exit(EXIT_FAILURE);
    }
    else printf("\n>Connected\n");
     
    //keep communicating with server
    
    printf("Hi, who are you? Enter name to proceed: ");
    scanf("%s",name);
    send(sock,name,strlen(name)+1,0);
    printf("***Instructions: Type in the format < ID > < : > < message >, where ID is the client you want to chat***\n");
    pthread_t thread;
    printf("Type: ");
    pthread_create(&thread, NULL, rece, (void *)(intptr_t)&sock);
    send_func(sock);
    pthread_join(thread, NULL);
    close(sock);
    return 0;
}