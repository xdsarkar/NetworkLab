#include<stdio.h>
#include<string.h>
#include<stdarg.h>
#include <ctype.h>
#include "packunpack.h"

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
