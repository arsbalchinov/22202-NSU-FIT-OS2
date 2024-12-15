#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>

#include "queue.h"

void *qmonitor(void *arg) {
	queue_t *q = (queue_t *)arg;
	printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

	while (1) {
		queue_print_stats(q);
		sleep(1);
	}
	return NULL;
}

queue_t* queue_init(int max_count) {
	int err;

	queue_t *q = malloc(sizeof(queue_t));
	if (!q) {
		printf("Cannot allocate memory for a queue\n");
		abort();
	}
	if (sem_init(&q->full, PTHREAD_PROCESS_PRIVATE, 0) != 0) {
		printf("Error: sem init (full)!\n");
		abort();
	}
	if (sem_init(&q->empty, PTHREAD_PROCESS_PRIVATE, max_count) != 0) {
        printf("Error: sem init (empty)!\n");
        abort();
    }
	if (sem_init(&q->lock, PTHREAD_PROCESS_PRIVATE, 1) != 0) {
		printf("Error: sem init (lock)!\n");
		abort();
	}

	q->first = NULL;
	q->last = NULL;
	q->max_count = max_count;
	q->count = 0;

	q->add_attempts = q->get_attempts = 0;
	q->add_count = q->get_count = 0;

	err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
	if (err) {
		printf("queue_init: pthread_create() failed: %s\n", strerror(err));
		abort();
	}
	return q;
}

void queue_destroy(queue_t *q) {
	pthread_cancel(q->qmonitor_tid);
    pthread_join(q->qmonitor_tid, NULL);

	sem_destroy(&q->full);
	sem_destroy(&q->empty);
	sem_destroy(&q->lock);

	while (q->first != NULL) {
		qnode_t* tmp = q->first;
		q->first = q->first->next;
		free(tmp);
	}

	free(q);
}

int queue_add(queue_t *q, int val) {
	sem_wait(&q->empty);

	q->add_attempts++;
	assert(q->count <= q->max_count);

	qnode_t *new = malloc(sizeof(qnode_t));
	if (!new) {
		printf("Cannot allocate memory for new node\n");
		abort();
	}
	sem_wait(&q->lock);

	new->val = val;
	new->next = NULL;

	if (!q->first)
		q->first = q->last = new;
	else {
		q->last->next = new;
		q->last = q->last->next;
	}

	q->count++;
	q->add_count++;

	sem_post(&q->lock);
	sem_post(&q->full);
	return 1;
}

int queue_get(queue_t *q, int *val) {
	sem_wait(&q->full);

	q->get_attempts++;
	assert(q->count >= 0);

	sem_wait(&q->lock);

	qnode_t *tmp = q->first;

	*val = tmp->val;
	q->first = q->first->next;

	free(tmp);
	q->count--;
	q->get_count++;

	sem_post(&q->lock);
	sem_post(&q->empty);
	return 1;
}

void queue_print_stats(queue_t *q) {
	printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
		q->count,
		q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
		q->add_count, q->get_count, q->add_count -q->get_count);
}
