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

////////////////////////////////////////////////////////

void packi8(unsigned char *buf, unsigned int i)
{
    *buf++ = i;
}

void packi16(unsigned char *buf, unsigned int i)
{
    *buf++ = i>>8; *buf++ = i;
}

void packi32(unsigned char *buf, unsigned long int i)
{
    *buf++ = i>>24; *buf++ = i>>16;
    *buf++ = i>>8; *buf++ = i;
}

////////////////////////////////////////////////////////

unsigned int unpacku8(unsigned char *buf)
{
    return ((unsigned int)buf[0]);
}

unsigned int unpacku16(unsigned char *buf)
{
    return ((unsigned int)buf[0]<<8) | buf[1];
}

unsigned long int unpacku32(unsigned char *buf)
{
    return ((unsigned long int)buf[0]<<24) |
            ((unsigned long int)buf[1]<<16) |
            ((unsigned long int)buf[2]<<8) |
            buf[3];
}

////////////////////////////////////////////////////////

unsigned int pack(unsigned char *buf, char *format, ...) //pack(buf, "UHL")
{
    va_list ap;

    unsigned int U;
    unsigned int H;
    unsigned long int L;

    char *s;
    unsigned int len;

    unsigned int size = 0;

    va_start(ap, format);

    for(; *format != '\0'; format++)
    {
        switch(*format)
        {
            case 'U'://8 bit unsigned
                size += 1;
                U = va_arg(ap, unsigned int);
                packi8(buf, U);
                buf += 1;
                break;

            case 'H'://16 bit unsigned
                size += 2;
                H = va_arg(ap, unsigned int);
                packi16(buf, H);
                buf += 2;
                break;

            case 'L'://32 bit unsigned
                size += 4;
                L = va_arg(ap, unsigned long int);
                packi32(buf, L);
                buf += 4;
                break;

            case 's'://string
                s = va_arg(ap, char*);
                len = strlen(s);
                size += len+2;
                packi16(buf,len);
                buf += 2;
                memcpy(buf, s, len);
                buf += len;
                break;
        }   
    }
    va_end(ap);
    return size;
}

////////////////////////////////////////////////////////

void unpack(unsigned char *buf, char *format, ...)
{
    va_list ap;
    unsigned int *U;
    unsigned int *H;
    unsigned long int *L;
    
    char *s;
    unsigned int len, maxstrlen=0, count;

    va_start(ap,format);

    for(; *format != '\0'; format++)
    {
        switch(*format)
        {
            case 'U': //8 bit unsigned
                U = va_arg(ap, unsigned int*);
                *U = unpacku8(buf);
                buf += 1;
                break;

            case 'H'://16 bit unsigned
                H = va_arg(ap, unsigned int*);
                *H = unpacku16(buf);
                buf += 2;
                break;

            case 'L'://32 bit unsigned
                L = va_arg(ap, unsigned long int*);
                *L = unpacku32(buf);
                buf += 4;
                break;

            case 's'://string
                s = va_arg(ap, char*);
                len = unpacku16(buf);
                buf += 2;
                if (maxstrlen > 0 && len > maxstrlen) count = maxstrlen - 1;
                else count = len;
                memcpy(s, buf, count);
                s[count] = '\0';
                buf += len;
                break;
            
            default:
                if (isdigit(*format)) 
                {
                    maxstrlen = maxstrlen * 10 + (*format-'0');
                }
        }
        if (!isdigit(*format)) maxstrlen = 0;
    }
    va_end(ap);
}

////////////////////////////////////////////////////////

uint32_t getMicrotime()
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
    //arg3: p
    //arg4: ttl
    //arg5: numpacks

    int sock, numbytes;
    char *ip = argv[1];
    short int PORT=(short)atoi(argv[2]);
    int p=atoi(argv[3]); //P
    int no_pack=atoi(argv[5]);
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
    uint8_t ttl = (uint8_t)atoi(argv[4]); //1 byte
    uint8_t ttl2, ttl3; //1 byte
    uint32_t time32, time_now32; //3 byte
    uint64_t time64, time_now64;
    uint32_t rtt=0, average_rtt=0; //3 byte each    
    char payload[p]; //payload, max = 1300
    unsigned char buf[1307];
    char s[p+1];
    for(int j=0; j<(p); j++) 
        s[j]='X';
    s[p]='\0';
    //char *s = "Sending packed data, stuffed with seq, ttl, timestamp and payload";
    int32_t sizePacket;
    for(int i=1; i<=no_pack; i++)
    {
        ttl2 = (uint8_t)atoi(argv[4]);
        ttl3 = (uint8_t)atoi(argv[4]);
        time64 = getMicrotime();
        time32 = time64 & 0xFFFFFFFF; //change to 32 bit
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
            ttl3--;
            if(ttl3 == 0) break;
        }
        time_now64 = getMicrotime();
        time_now32 = time_now64 & 0xFFFFFFFF; //change to 32 bit
        rtt = time_now32 - time32;
        average_rtt = average_rtt + rtt;
    }
    ofstream outfile;
    outfile.open("test.txt", std::ios_base::app);
    float avg = (float)average_rtt/no_pack;
    outfile<<average_rtt<<" "<<avg<<endl;
    printf("The Average RTT delay is %f usec and %d packets\n", avg, no_pack);
    close(sock);
    return 0;
}