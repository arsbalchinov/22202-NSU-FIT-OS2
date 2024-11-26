#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

void *mythread(void *arg) {
	//pthread_detach(pthread_self());
	//char *str = "hello world";
	printf("mythread [%d %d %d %ld]: Hello from mythread!\n", getpid(), getppid(), gettid(), pthread_self());
	//return str;
	return NULL;
}

int main() {
	pthread_t tid;
	int err;
	char *thread_result;

	//pthread_attr_t attr;
	//err = pthread_attr_init(&attr);
	//err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	printf("main [%d %d %d]: Hello from main!\n\n", getpid(), getppid(), gettid());

	while (1) {
		err = pthread_create(&tid, NULL, mythread, NULL);
		//err = pthread_create(&tid, &attr, mythread, NULL);	//Создали detached поток
		if (err) {
    		printf("main: pthread_create() failed: %s\n", strerror(err));
			return -1;
		}
	}
	//pthread_attr_destroy(&attr);
	pthread_join(tid, &thread_result);	//Подключаемся к потоку и получаем результат в thread_result
	printf("Thread tid: '%ld'\n\n", tid);
	printf("\n%p\n", thread_result);
	printf("Thread result - %s\n", thread_result);
	sleep(10);
	return 0;
}

