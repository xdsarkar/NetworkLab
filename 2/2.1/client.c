#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<fcntl.h> // for open
#include<unistd.h> // for close
#include<stdlib.h> //exit

#define PORT 8000 //user_define free port
#define max_len 2500 //length of message buffer

int main(int argc , char *argv[])
{
    int sock, numbytes;
    struct sockaddr_in server;
    char message[max_len], server_reply[max_len];
     
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
    while(1)
    {
        printf("\n>Type to chat: ");
        scanf("%s" , message);
         
        //Send some data
        if(send(sock, message, strlen(message), 0) < 0)
        {
            printf("\n>Error: send (sending message) failed");
            return 0;
        }
        else printf(">Sent message, waiting for reply...");

        //Receive a reply from the server
        numbytes = recv(sock, server_reply, max_len, 0);
        if(numbytes < 0)
        {
            printf("\n>Error: recv (recieve message) failed");
            break;
        }
        else
        {
            server_reply[numbytes]='\0';
            //returns back the server reply (same reply) 
            printf("\n>Server reply: %s\n", server_reply);
            fflush(stdout);
            server_reply[0]='\0';
        }
    }
     
    close(sock);
    return 0;
}