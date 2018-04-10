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
#define max_len 2500

char client_message[max_len]; //client_msg buffer
int sock;

void* send(void *arg)
{
    while(true)
    {
        cout<<"Client: ";
        string str; cin>>str;
        strcpy(client_message, str.c_str());
        send(sock, (char*)&client_message, strlen(client_message), 0);
        if(str=="end_client")
        {
            cout<<"Client side have ended the chat"<<endl;
            pthread_cancel(pthread_self()); 
            return NULL;
            //pthread_exit(NULL);
        }
        else client_message[0]='\0';  
    }
    //pthread_exit(NULL);
}

void* receive(void *arg)
{
    while(true)
    {
        int re;
        re=read(sock, client_message, max_len);
        if(!strcmp(client_message, "end_server"))
        {
            cout<<"Server side have ended the chat"<<endl;
            pthread_cancel(pthread_self()); 
            return NULL;
            //pthread_exit(NULL);
        }
        else
        {
            client_message[re]='\0';
            cout<<"Server: "<<client_message<<endl;
            client_message[0]='\0';
        }
    }
    //pthread_exit(NULL);
}

int main(int argc , char *argv[])
{
    int PORT;
    if(argc<2) PORT=prePO;
    else PORT=atoi(argv[1]);

    int numbytes;
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
    int thread;
    pthread_t tid[2];

    thread = pthread_create(&tid[0], NULL, send, NULL);

    if(thread!=0)          
    {
        cout<<"Error while sending"<<thread<<endl;
        return 0;
    }

    thread = pthread_create(&tid[1], NULL, receive, NULL);  
    
    if(thread!=0)          
    {
        cout<<"Error while receiving"<<thread<<endl;
        return 0;
    }

    pthread_join(tid[0],NULL);
    pthread_join(tid[1],NULL);
     
    close(sock);
    pthread_exit(NULL);
}

// **Below Info Source: Stack Exchange** //
/*Since you are using TCP, this is having some limitations. TCP is a byte oriented protocol. So ideally you cannot expect to get all the data in a single "recv". You need to run recv in loop even to grab all the data in a line, if the line is arbitrarily large. */
