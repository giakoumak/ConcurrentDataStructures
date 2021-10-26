/*
 * Author: Giannis Giakoumakis
 * Contact: giannis.m.giakoumakis@gmail.com
 */

#include <stdlib.h>
#include <stdio.h>

#include "../include/conqueue.h"

void
initqueue(struct queue *q)
{
	int e;
	struct queue_node *node;

	node = malloc(sizeof(struct queue_node));
	if (node == NULL) {
		printf("malloc() failed\n");
		exit(EXIT_FAILURE);
	}

	/* Sentinel node values */
	node->inf.producerID = -1;
	node->inf.timestamp = -1;
	node->next = NULL;

	/* Initialization of the queue */
	q->Head = node;
	q->Tail = node;
	e = pthread_mutex_init(&q->head_lock, NULL);
	if (e != 0) {
		printf("pthread_mutex_init() failed\n");
		exit(EXIT_FAILURE);
	}
	e = pthread_mutex_init(&q->tail_lock, NULL);
	if (e != 0) {
		printf("pthread_mutex_init() failed\n");
		exit(EXIT_FAILURE);
	}
}

void
enqueue(struct queue *q, int pid, int ts)
{
	struct queue_node *node;

	node = malloc(sizeof(struct queue_node));
	if (node == NULL) {
		printf("malloc() failed\n");
		exit(EXIT_FAILURE);
	}

	/* Initialize the fields of the new node */
	node->inf.producerID = pid;
	node->inf.timestamp = ts;
	node->next = NULL;

	/* Ensure only one process interacts with the tail */
	pthread_mutex_lock(&q->tail_lock);
	q->Tail->next = node;
	q->Tail = node;
#ifdef _VERBOSE
	printf("Tail={producerID=%d timestamp=%d}\n",
	    q->Tail->inf.producerID, q->Tail->inf.timestamp);
#endif /* _VERBOSE */
	pthread_mutex_unlock(&q->tail_lock);
}

struct info *
dequeue(struct queue *q)
{
	struct queue_node *tmp;
	struct info *result = NULL;

	pthread_mutex_lock(&q->head_lock);
	if (q->Head->next != NULL) {
		result = malloc(sizeof(struct info));
		if (result == NULL) {
			printf("malloc() failed\n");
			exit(EXIT_FAILURE);	
		}
		result->producerID = q->Head->next->inf.producerID;
		result->timestamp = q->Head->next->inf.timestamp;
		tmp = q->Head;
		q->Head = q->Head->next;
		free(tmp);
#ifdef _VERBOSE
		printf("Result={producerID=%d timestamp=%d}\n",
		    result->producerID, result->timestamp);
		printf("Head={producerID=%d timestamp=%d}\n",
		    q->Head->inf.producerID, q->Head->inf.timestamp);
#endif /* _VERBOSE */
	}
	pthread_mutex_unlock(&q->head_lock);

	return result;
}

#ifdef _UTEST

#define NUM_THREADS 4

void *
produce(void *arg)
{
	struct producer_attr *attr;
	pthread_t self_tid;
	int self_id, i, timestamp;

	attr = (struct producer_attr *)arg;
	
	self_tid = pthread_self();
	for (i = 0; i < NUM_THREADS; i++) {
		if (self_tid == attr->tid[i]) {
			self_id = i;
			break;
		}
	}
	printf("self_id=%d\n", self_id);

	for (i = 0; i <= NUM_THREADS - 1; i++) {
		timestamp = (i * NUM_THREADS) + self_id;
		enqueue(attr->queue, self_id, timestamp);
	}
}

void *
consume(void *arg)
{
	struct consumer_attr *attr;
	struct info *result = NULL;

	attr = (struct consumer_attr *)arg;

	while (1) {
		result = dequeue(attr->queue);
		if (result == NULL)
			break;
	}
}

int
main()
{
	pthread_t tid[NUM_THREADS];
	struct queue q;
	struct producer_attr p_attr;
	struct consumer_attr c_attr;
	int i, e;

	initqueue(&q);

	printf("This is just a test. A queue has been initialized.\n");
	printf("Head={producerID=%d timestamp=%d}\n",
	    q.Head->inf.producerID, q.Head->inf.timestamp);
	printf("Tail={producerID=%d timestamp=%d}\n",
	    q.Tail->inf.producerID, q.Tail->inf.timestamp);	

	/* Spawn threads to test concurrent enqueues */
	p_attr.queue = &q;
	p_attr.tid = tid;
	for (i = 0; i < NUM_THREADS; i++) {
		e = pthread_create(&tid[i], NULL, produce,
		    (void *)&p_attr);
		if (e != 0) {
			printf("pthread_create() failed\n");
			exit(EXIT_FAILURE);
		}
	}
	for (i = 0; i < NUM_THREADS; i++) {
		e = pthread_join(tid[i], NULL);
		if (e != 0) {
			printf("pthread_join() failed\n");
			exit(EXIT_FAILURE);
		}
	}

	/* Spawn threads to test concurrent dequeues */
	c_attr.queue = &q;
	for (i = 0; i < NUM_THREADS; i++) {
		e = pthread_create(&tid[i], NULL, consume,
		    (void *)&c_attr);
		if (e != 0) {
			printf("pthread_create() failed\n");
			exit(EXIT_FAILURE);
		}
	}
	for (i = 0; i < NUM_THREADS; i++) {
		e = pthread_join(tid[i], NULL);
		if (e != 0) {
			printf("pthread_join() failed\n");
			exit(EXIT_FAILURE);
		}
	}

	return 0;
}

#endif /* _UTEST */
