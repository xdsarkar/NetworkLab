#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdarg.h>
#include<inttypes.h>
#include<sys/time.h>
#include<fstream>
#include <ctype.h>
#include "packunpack.h"

uint64_t getMicrotime()
{
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    return currentTime.tv_sec *(uint64_t)1000000 + currentTime.tv_usec;
}

int main(int argc , char *argv[])
{
    short int PORT;
    PORT=(short)atoi(argv[1]);
    int socket_desc, client_sock, read_size, yes=1;
    socklen_t addr_len; 
    struct sockaddr_in server, client;

    socket_desc = socket(PF_INET , SOCK_DGRAM , 0); /* socket descriptor */

    if (socket_desc == -1) printf("\nCould not create socket");
    printf("\n>Socket created");
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET; /* set the type of connection to TCP/IP */
    server.sin_addr.s_addr = INADDR_ANY; /* set our address to any interface */
    server.sin_port = htons(PORT); /* set the server port number */
     
    /* Binding */
    if(bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("\nError: Binding Failed");
        return 1;
    }
    else printf("\n>Binding process completed\n");

    addr_len = sizeof(struct sockaddr_in);
    uint8_t ttl, ttl2; //1 byte
    uint16_t seq_no; //2 byte
    uint32_t time_st; //3 byte
    char payload[1300];
    unsigned char buf[1307];
    uint32_t sizePacket;
    while(1)
    {
        recvfrom(socket_desc,&buf,1307,0,(struct sockaddr*)&client,&addr_len);
        unpack(buf, "HLU1300s", &seq_no, &time_st, &ttl, payload);
        buf[0]='\0';
        ttl=ttl-1;
        ttl2 = ttl;
        /* printf("% " PRId16 " % " PRId32 " % " PRId8 " %s\n",seq_no, time_st, ttl2, payload); */
        sizePacket = pack(buf, "HLUs", (uint16_t)seq_no, (uint32_t)time_st, (uint8_t)ttl2, payload);
        sendto(socket_desc,&buf,(int)sizePacket,0,(struct sockaddr*)&client,addr_len);
        buf[0]='\0';
    }
    close(socket_desc); /* closes the original socket for reuse again */
    return 0;
}