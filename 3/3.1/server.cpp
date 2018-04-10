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

unsigned int pack(unsigned char *buf, char *format, ...)
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
            case 'U':
                size += 1;
                U = va_arg(ap, unsigned int);
                packi8(buf, U);
                buf += 1;
                break;

            case 'H':
                size += 2;
                H = va_arg(ap, unsigned int);
                packi16(buf, H);
                buf += 2;
                break;

            case 'L':
                size += 4;
                L = va_arg(ap, unsigned long int);
                packi32(buf, L);
                buf += 4;
                break;

            case 's':
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
            case 'U':
                U = va_arg(ap, unsigned int*);
                *U = unpacku8(buf);
                buf += 1;
                break;

            case 'H':
                H = va_arg(ap, unsigned int*);
                *H = unpacku16(buf);
                buf += 2;
                break;

            case 'L':
                L = va_arg(ap, unsigned long int*);
                *L = unpacku32(buf);
                buf += 4;
                break;

            case 's':
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
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
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
    unsigned char buf[1308];
    int32_t sizePacket;
    while(1)
    {
        recvfrom(socket_desc,&buf,sizeof(buf),0,(struct sockaddr*)&client,&addr_len);
        unpack(buf, "HLU1300s", &seq_no, &time_st, &ttl, payload);
        buf[0]='\0';
        ttl=ttl-1;
        ttl2 = ttl;
        /* printf("% " PRId8 " %s\n", ttl2, payload); */
        sizePacket = pack(buf, "HLUs", (uint16_t)seq_no, (uint32_t)time_st, (uint8_t)ttl2, payload);
        sendto(socket_desc,&buf,(int)sizePacket,0,(struct sockaddr*)&client,addr_len);
        buf[0]='\0';
    }
    close(socket_desc);
    /* closes the original socket for reuse again */
    return 0;
}