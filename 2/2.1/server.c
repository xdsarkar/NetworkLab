#include<stdio.h> //Input/ Output handler
#include<string.h>    //strlen, memset, bzero
#include<sys/types.h> //This header file contains definitions of a number of data types used in system calls. These types are used in the next two include files.
#include<sys/socket.h> //The header file socket.h includes a number of definitions of structures needed for sockets.
#include<arpa/inet.h> //inet_addr The header file netinet/in.h contains constants and structures needed for internet domain addresses.
#include<unistd.h>    //write
#include<stdlib.h>

#define PORT 8000 //user defined free port
#define max_len 2500
int main(int argc , char *argv[])
{
    int socket_desc, client_sock, addr_len, read_size, yes=1;
    struct sockaddr_in server, client;
    char client_message[max_len]; //client_msg buffer

    /*
    
    //generic structures

    struct in_addr {
        unsigned long s_addr; // Internet address (32 bits)
    }

    struct sockaddr_in
    {
        unsigned short sin_family; // Internet protocol (AF_INET)
        unsigned short sin_port; // Address port (16 bits)
        struct in_addr sin_addr; // Internet address (32 bits)
        char sin_zero[8]; /* Not used
    }

    struct sockaddr
    {
        unsigned short sa_family; // Address family (e.g. AF_INET)
        char sa_data[14]; // Family-specific address information
    }

    */

    //Create socket

    /* AF_INET: sockaddr_in, PF_INET: socket descriptor (socket ()) */
    socket_desc = socket(PF_INET , SOCK_STREAM , 0); //socket descriptor
    if (socket_desc == -1) printf("\nCould not create socket");
    printf("\n>Socket created");
    
     /* zero the struct before filling the fields */
    memset(&server, 0, sizeof(server));
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET; // set the type of connection to TCP/IP
    server.sin_addr.s_addr = INADDR_ANY; //set our address to any interface
    server.sin_port = htons(PORT); //set the server port number
    
    //setsockopt is use the binding with success,  as sometimes kernel uses the address even after exit, but with this we can avoid that
    
    /*
    if (setsockopt(socket_desc,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(yes) == -1))
    {
        perror("setsockopt");
        exit(1);
    }
    */
     
    //Binding
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        //perror is of not much advantegous here, in case of thread safety perror is preferred
        perror("\nError: Binding Failed");
        return 1;
    }
    printf("\n>Binding process completed");
     
    //Listening
    listen(socket_desc, 1); //queue of 1 client

    addr_len = sizeof(struct sockaddr_in);
     
    //accept connection from an incoming client
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&addr_len);
    if (client_sock < 0)
    {
        perror("\nError: accept() failed");
        return 1;
    }
    else printf("\n>Connection connected");

    printf("\n>Waiting for incoming message...\n>Message recieved and sent again to client...");
    //Receive a message from client
    while((read_size = recv(client_sock, client_message, max_len, 0)) > 0)
    {
        printf("\n>Waiting for incoming message...\n>Message recieved and sent again to client...");
        //Send the message back to client
        client_message[read_size]='\0';
	    printf("\n>Client reply: %s\n", client_message);
        write(client_sock , client_message , strlen(client_message));
        client_message[0]='\0';
    }
     
    if(read_size == 0)
    {
        printf("\nClient disconnected");
        fflush(stdout);
    }
    else if(read_size == -1) perror("\nError: recv (recieve) failed");

    close(client_sock); //closes the accept created socket
    close(socket_desc); //closes the original socket for reuse again
    
    return 0;
}

// **Below Info Source: Stack Exchange** //
/*Since you are using TCP, this is having some limitations. TCP is a byte oriented protocol. So ideally you cannot expect to get all the data in a single "recv". You need to run recv in loop even to grab all the data in a line, if the line is arbitrarily large. */
