#ifndef PACKUNPACK_H_INCLUDED
#define PACKUNPACK_H_INCLUDED
void packi8(unsigned char *buf, unsigned int i);
void packi16(unsigned char *buf, unsigned int i);
void packi32(unsigned char *buf, unsigned long int i);
unsigned int unpacku8(unsigned char *buf);
unsigned int unpacku16(unsigned char *buf);
unsigned long int unpacku32(unsigned char *buf);
unsigned int pack(unsigned char *buf, char *format, ...);
void unpack(unsigned char *buf, char *format, ...);
#endif