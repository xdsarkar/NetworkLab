#include<stdio.h>
#include<sys/time.h>
#include<iostream>
#include<inttypes.h>
using namespace std;

uint64_t GetTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

unsigned long getTimeStamp()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
	return time_in_micros;
}

int main()
{
	cout<<GetTimeStamp();
	return 0;
}