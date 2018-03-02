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
    return currentTime.tv_sec * (uint32_t)1000000 + currentTime.tv_usec;
}

using namespace std;

int main(int argc , char *argv[])
{
    //arg1: ip
    //arg2: port
    //arg3: ttl
    //arg4: numpacks

    int sock, numbytes;
    char *ip = argv[1];
    short int PORT=(short)atoi(argv[2]);
    //int p=atoi(argv[3]); //P
    uint8_t ttl = (uint8_t)atoi(argv[3]); //1 byte
    int no_pack=atoi(argv[4]);
    socklen_t server_len;
    struct sockaddr_in server;
    server_len = sizeof(struct sockaddr_in);

    sock = socket(PF_INET , SOCK_DGRAM , 0);
    if (sock == -1)
    {
        printf(">Error: Socket creation failed\n");
    }
    else printf(">Socket created\n");
     
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    printf("Wait for the results...\n\n");
    uint16_t seq_no; //2 byte
    uint8_t ttl2, ttl3; //1 byte
    uint32_t time32, time_now32; //3 byte
    uint64_t time64, time_now64;
    uint32_t rtt=0, average_rtt=0; //3 byte each    
    unsigned char buf[1307];

    for(int p=100; p<=1300; p=p+100)
    {
        char payload[p+1]; //payload, max = 1300
        char s[p+1];
        /*for(int j=0; j<(p); j++) 
            s[j]='X';
        */
        s[p+1]='\0';
        /* char *s = "Sending packed data, stuffed with seq, ttl, timestamp and payload"; */
        uint32_t sizePacket;
        for(int i=1; i<=no_pack; i++)
        {
            ttl2 = (uint8_t)atoi(argv[3]);
            ttl3 = (uint8_t)atoi(argv[3]);
            time64 = getMicrotime();
            time32 = time64 & 0xFFFFFFFF; //change to 32 bit
            /* and-ing it with 0xffffffff makes sure any bits over the 32 are zeroed out */
            while(1)
            {
                ttl2 = ttl3;
                /*Pack and send*/
                sizePacket = pack(buf,"HLUs", (uint16_t)i, (uint32_t)time32, (uint8_t)ttl2, s);
                sendto(sock,buf,(int)sizePacket,0,(struct sockaddr*)&server,server_len);
                buf[0]='\0';
                /*Recieve and unpack */
                recvfrom(sock,buf,sizeof(buf),0,(struct sockaddr*)&server,&server_len);
                unpack(buf, "HLU1300s", &seq_no, &time32, &ttl, payload);
                buf[0]='\0';
                ttl3 = ttl;
                ttl--;
                if(ttl == 0) break;
            }
            time_now64 = getMicrotime();
            time_now32 = time_now64 & 0xFFFFFFFF; //change to 32 bit
            /* and-ing it with 0xffffffff makes sure any bits over the 32 are zeroed out */
            rtt = time_now32 - time32;
            average_rtt = average_rtt + rtt;
        }
        ofstream outfile;
        outfile.open("test.txt", std::ios_base::app);
        float avg = (float)average_rtt/no_pack;
        outfile<<"TTL: "<<atoi(argv[3])<<", Payload Size: "<<p<<", Average RTT: "<<avg<<", Packets: "<<no_pack<<endl;
        printf("Payload Size: (%d) >> The Avg RTT is %f usec for %d packets\n", p, avg, no_pack);
    }
    close(sock);
    return 0;
}