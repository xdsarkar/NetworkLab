#include<stdio.h>
#include<pthread.h>

//thread function def
void* threadFunction(void* args)
{
	while(1) printf("I am thread function\n");
}

int main()
{
	//create thread_id
	pthread_t id;
	int ret;

	//create thread
	ret = pthread_create(&id,NULL,&threadFunction,NULL);
	if(ret == 0) printf("Thread created successfully.\n");
	else
	{
		printf("Thread not create.\n");
		return 0; //return from main
	}
	while(1) printf("I am main function.\n");
	return 0;
}