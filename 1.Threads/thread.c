#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

int global = 1000;

void *mythread(void *arg) {
	int local = 1;
	static int static_var = 10;
	const int const_var = 100;
	printf("mythread [%d %d %d %ld]: Hello from mythread!\n", getpid(), getppid(), gettid(), pthread_self());
	printf("local - '%p' - %d\nstatic - '%p' - %d\nconst - '%p' - %d\nglobal - '%p' - %d\n", &local, local, &static_var, static_var, &const_var, const_var, &global, global);
	local *= 3;
	global *= 7;
	printf("local - '%p' - %d\nglobal - '%p' - %d\n", &local, local, &global, global);
	sleep(60);
	return NULL;
}

int main() {
	pthread_t tid[5];
	int err[5];

	printf("main [%d %d %d]: Hello from main!\n\n", getpid(), getppid(), gettid());
	sleep(10);

	for (int i = 0; i < 5; i++) {
		err[i] = pthread_create(&tid[i], NULL, mythread, NULL);
		if (err[i]) {
	    	printf("main: pthread_create() #%d failed: %s\n", i, strerror(err[i]));
			return -1;
		}
		//pthread_join(tid[i], NULL);
		sleep(1);
		printf("Thread #%d: tid = '%ld'\n\n", i, tid[i]);
	}

	sleep(100);
	return 0;
}
