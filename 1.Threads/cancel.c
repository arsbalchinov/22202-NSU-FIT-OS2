#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#define HELLO_STR "hello world"

//int counter = 0;

void exit_func(void *ptr) {
	free(ptr);
	printf("memory freed successfully!\n");
}

void *mythread(void *arg) {
	//printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
	char *str = malloc(25);
	pthread_cleanup_push(exit_func, str);
	strcpy(str, HELLO_STR);
	while (1) {
		//counter++;
		//pthread_testcancel();
		//printf("%d\n", counter);
		printf("%s\n", str);
	}
	pthread_cleanup_pop(1);
	return NULL;
}

int main() {
	pthread_t tid;
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

	err = pthread_create(&tid, NULL, mythread, NULL);
	if (err) {
	    printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	sleep(3);
	printf("\n\nCANCEL!!!\n\n");
	pthread_cancel(tid);
	//printf("\n%d\n", counter);
	pthread_join(tid, NULL);
	return 0;
}
