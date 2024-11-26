#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct mystruct {
	int num;
	char *str;
} mystruct;

void *mythread(void *arg) {
	printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
	mystruct *param = (mystruct*) arg;

	printf("%d\n%s\n", param->num, param->str);
	return NULL;
}

int main() {
	pthread_t tid;
	int err;
	mystruct param = {42, "hello"};

	printf("main [%d %d %d]: Hello from main!\n\n", getpid(), getppid(), gettid());
	printf("struct mystruct - %p\n", &param);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	err = pthread_create(&tid, &attr, mythread, &param);
	if (err) {
	    printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	pthread_attr_destroy(&attr);
	//pthread_join(tid, NULL);
	sleep(30);

	return 0;
}
