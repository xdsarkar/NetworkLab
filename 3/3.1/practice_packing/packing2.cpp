#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<stdarg.h>
#include<inttypes.h>

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

unsigned int pack(unsigned char *buf, char *format, ...)
{
    va_list ap;

    uint8_t u;
    uint16_t h;
    uint32_t l;
    char *s;
    unsigned int len;
    unsigned int size = 0;

    va_start(ap, format);

    for(; *format != '\0'; format++)
    {
        switch(*format)
        {
            case 'u':
                size += 2;
                u = va_arg(ap, uint8_t);
                packi8(buf, u);
                buf += 2;
                break;

            case 'h':
                size += 2;
                h = va_arg(ap, uint16_t);
                packi16(buf, h);
                buf += 2;
                break;

            case 'l':
                size += 4;
                l = va_arg(ap, uint32_t);
                packi32(buf, l);
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

    uint8_t *u;
    uint16_t *h;
    uint32_t *l;

    char *s;
    unsigned int len, maxstrlen=0, count;

    va_start(ap,format);

    for(; *format != '\0'; format++)
    {
        switch(*format)
        {
            case 'u':
                u = va_arg(ap, uint8_t*);
                *u = unpacku8(buf);
                buf += 2;
                break;

            case 'h':
                h = va_arg(ap, uint16_t*);
                *h = unpacku16(buf);
                buf += 2;
                break;

            case 'l':
                l = va_arg(ap, uint32_t*);
                *l = unpacku32(buf);
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

int main()
{
    //using namespace std;
    unsigned char buf[1024];
    uint16_t a=123,b=345,c,d;
    uint16_t packetsize;
    char *s="Packed data, done and delivered";
    char s2[1300];
    packetsize=pack(buf,"hhs", (uint16_t)a, (uint16_t)b, s);
    unpack(buf, "hh1300s", &c, &d, s2);
    printf ("%" PRId16 " %" PRId16 "\n", a, b);
    printf ("%" PRId16 " %" PRId16 " %s", c, d, s2);
    return 0;
}