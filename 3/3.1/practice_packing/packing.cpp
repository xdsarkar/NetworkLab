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

int unpacki16(unsigned char *buf)
{
	unsigned int i2 = ((unsigned int)buf[0]<<8) | buf[1];
	int i;
	// change unsigned numbers to signed
	if (i2 <= 0x7fffu) 
	{ 
		i = i2; 
	}
	else 
	{ 
		i = -1 - (unsigned int)(0xffffu - i2); 
	}
	return i;
}

unsigned int unpacku16(unsigned char *buf)
{
	return ((unsigned int)buf[0]<<8) | buf[1];
}

long int unpacki32(unsigned char *buf)
{
	unsigned long int i2 = ((unsigned long int)buf[0]<<24) |
	((unsigned long int)buf[1]<<16) |
	((unsigned long int)buf[2]<<8) |
	buf[3];
	long int i;
	// change unsigned numbers to signed
	if (i2 <= 0x7fffffffu) 
	{ 
		i = i2; 
	}
	else 
	{ 
		i = -1 - (long int)(0xffffffffu - i2); 
	}
	return i;
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

	int h;
	unsigned int H;

	long int l;
	unsigned long int L;

	char *s;
	unsigned int len;

	unsigned int size = 0;

	va_start(ap, format);

	for(; *format != '\0'; format++)
	{
		switch(*format)
		{

			case 'h':
				size += 2;
				h = va_arg(ap, int);
				packi16(buf, h);
				buf += 2;
				break;

			case 'H':
				size += 2;
				H = va_arg(ap, unsigned int);
				packi16(buf, H);
				buf += 2;
				break;

			case 'l':
				size += 4;
				l = va_arg(ap, long int);
				packi32(buf, l);
				buf += 4;
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

	int *h;
	unsigned int *H;

	long int *l;
	unsigned long int *L;

	char *s;
	unsigned int len, maxstrlen=0, count;

	va_start(ap,format);

	for(; *format != '\0'; format++)
	{
		switch(*format)
		{
			case 'h':
				h = va_arg(ap, int*);
				*h = unpacki16(buf);
				buf += 2;
				break;

			case 'H':
				H = va_arg(ap, unsigned int*);
				*H = unpacku16(buf);
				buf += 2;
				break;

			case 'l':
				l = va_arg(ap, long int*);
				*l = unpacki32(buf);
				buf += 4;
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
				{ // track max str len
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
	int16_t a=123,b=345,c,d;
	int16_t packetsize;
	char *s="Packed data, done and delivered";
	char s2[1300];
	packetsize=pack(buf,"hhs", (int16_t)a, (int16_t)b, s);
	unpack(buf, "hh1300s", &c, &d, s2);
	printf ("%" PRId16 " %" PRId16 "\n", a, b);
	printf ("%" PRId16 " %" PRId16 " %s", c, d, s2);
	return 0;
}