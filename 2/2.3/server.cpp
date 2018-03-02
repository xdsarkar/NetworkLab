#include<stdio.h> //Input/ Output handler
#include<string.h>    //strlen, memset, bzero
#include<sys/types.h> //This header file contains definitions of a number of data types used in system calls. These types are used in the next two include files.
#include<sys/socket.h> //The header file socket.h includes a number of definitions of structures needed for sockets.
#include<arpa/inet.h> //inet_addr The header file netinet/in.h contains constants and structures needed for internet domain addresses.
#include<unistd.h>    //write
#include<stdlib.h>
#include<iostream>
#include<pthread.h>
#include<bits/stdc++.h>

using namespace std;

#define prePO 8000 //user defined free port
#define max_len 4500 //buffer_length
#define BACKLOG 5

void *rece(void *);

struct client_d //structure for client data identification
{
    char name[100];
    int socket_desc;
    int online;
    int id; //client id, socket description, online=0(offline) or 1(online)
    pthread_t tid;
}clients[BACKLOG];

int main(int argc , char *argv[])
{
    int PORT;
    if(argc<2) PORT=prePO;
    else PORT=atoi(argv[1]);
    int socket_desc, addr_len;
    struct sockaddr_in server, client;

    socket_desc = socket(PF_INET , SOCK_STREAM , 0); //socket descriptor
    if (socket_desc == -1) printf("\nCould not create socket");
    printf("\n>Socket created");
    
     /* zero the struct before filling the fields */
    memset(&server, 0, sizeof(server));
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET; // set the type of connection to TCP/IP
    server.sin_addr.s_addr = INADDR_ANY; //set our address to any interface
    server.sin_port = htons(PORT); //set the server port number
     
    //Binding
    if(bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        //perror is of not much advantegous here, in case of thread safety perror is preferred
        perror("\nError: Binding Failed");
        return 1;
    }

    printf("\n>Binding process completed\n");
     
    //Listening
    listen(socket_desc, BACKLOG); //queue of 5 client

    for(int i=0; i<BACKLOG; i++) clients[i].online=0; //offline

    for(int i=0; i<BACKLOG; i++)
    {
        addr_len = sizeof(struct sockaddr_in);
        int client_sock=accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&addr_len);
        if(client_sock < 0)
        {
            printf("Accept() failed\n");
            return 0;
        }
        else
        {
            char name[100];
            recv(client_sock, &name, sizeof(name),0);
            clients[i].socket_desc = client_sock;
            strcpy(clients[i].name,name);
            clients[i].online=1;
            clients[i].id=i;
            printf("<%s> starts the chat with <ID = %d>...\n",name,clients[i].id);
            pthread_create(&clients[i].tid, NULL, rece, (void *)(intptr_t)&clients[i].id);
        }
    }

    for(int i=0; i<BACKLOG; i++)
    {
        pthread_join(clients[i].tid, NULL);
    }
    
    close(socket_desc);
    return 0;
}

void *rece(void *sent_by)
{
    int sender=*(intptr_t *)sent_by;
    int i;
    char buff[max_len];
    while(1)
    {
        int b = recv(clients[sender].socket_desc,&buff,sizeof(buff),0);

        if(b<=0)
        {
            clients[sender].online=0;
            printf("%s has exited the chat\n",clients[sender].name);
            close(clients[sender].socket_desc);
            return 0;
        }
        else
        {
            int send_to_id;
            send_to_id = buff[0] - '0';
            memmove(buff, &(buff[2]), strlen(&(buff[2])));
            buff[strlen(buff) - 2] = '\0';

            char print_msg[max_len]="";
            char msg_self[100]="Why sending yourself an message, ";
            strcat(msg_self,clients[sender].name);
            strcat(msg_self,"?");

            strcat(print_msg,clients[sender].name);
            strcat(print_msg," << ");
            strcat(print_msg,buff);

            if(send_to_id!=sender && clients[send_to_id].online!=0)
            {
                send(clients[send_to_id].socket_desc,print_msg,strlen(print_msg)+1,0);
                print_msg[0]='\0';
            }
            else if(send_to_id == sender)
            {
                send(clients[send_to_id].socket_desc,msg_self,strlen(msg_self)+1,0);
                msg_self[0]='\0';
            }
        }
    }
    pthread_exit(NULL);
}