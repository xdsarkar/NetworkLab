#include<bits/stdc++.h>
using namespace std;
int main()
{
	char str[] = "0:abcdef";
	memmove(str, &(str[2]), strlen(&(str[2])));
	str[strlen(str) - 2] = '\0';
	printf("%sxxx\n", str);
}