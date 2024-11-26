#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>

void sigint_handler(int sig) {
	printf("mythread2 got signal SIGINT\n");
}

void *mythread1(void *arg) {
	printf("mythread1 [%d %d %d]: Hello from mythread1!\n", getpid(), getppid(), gettid());
	sigset_t set;
	sigfillset(&set);

	if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
		perror("mythread1 sigprocmask failed\n");
		pthread_exit(NULL);
	}

	printf("mythread1 is blocking all signals\n");
	sleep(10);	//Типа работает...
	return NULL;
}

void *mythread2(void *arg) {
	printf("mythread2 [%d %d %d]: Hello from mythread2!\n", getpid(), getppid(), gettid());

	signal(SIGINT, sigint_handler);

	printf("mythread2 is waiting for SIGINT\n");
	sleep(10);
	return NULL;
}

void *mythread3(void *arg) {
	printf("mythread3 [%d %d %d]: Hello from mythread3!\n", getpid(), getppid(), gettid());
	int sig;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGQUIT);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	if (sigwait(&set, &sig) != 0) {
		perror("mythread3 sigwait failed\n");
	}
	else {
		printf("mythread3 got signal SIGQUIT\n");
	}
	return NULL;
}

int main() {
	pid_t pid = getpid();
	pthread_t tid[3];
	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

	pthread_create(&tid[0], NULL, mythread1, NULL);
	pthread_create(&tid[1], NULL, mythread2, NULL);
	pthread_create(&tid[2], NULL, mythread3, NULL);

	sleep(1);

	kill(pid, SIGINT);
	sleep(5);
	pthread_kill(tid[2], SIGQUIT);

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);
	pthread_join(tid[2], NULL);

	return 0;
}
