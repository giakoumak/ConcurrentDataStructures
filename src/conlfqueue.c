/*
 * Author: Giannis Giakoumakis
 * Contact: giannis.m.giakoumakis@gmail.com
 */

#include <stdlib.h>
#include <stdio.h>

#include "../include/conlfqueue.h"

#define CAS __sync_bool_compare_and_swap

void
initlfqueue(struct lfqueue *q)
{
	int e;
	struct lfqueue_node *node;

	node = malloc(sizeof(struct lfqueue_node));
	if (node == NULL) {
		printf("malloc() failed\n");
		exit(EXIT_FAILURE);
	}

	/* Sentinel node values */
	node->inf.producerID = -1;
	node->inf.timestamp = -1;
	node->next = NULL;

	/* Initialization of the lock free queue */
	q->Head = node;
	q->Tail = node;
}

void
lfenqueue(struct lfqueue *q, int pid, int ts)
{
	struct lfqueue_node *next, *last, *node;

	node = malloc(sizeof(struct lfqueue_node));
	if (node == NULL) {
		printf("malloc() failed\n");
		exit(EXIT_FAILURE);
	}

	/* Initialize the fields of the new node */
	node->inf.producerID = pid;
	node->inf.timestamp = ts;
	node->next = NULL;

	while (1) {
		last = q->Tail;
		next = last->next;
		if (last == q->Tail) {
			if (next == NULL) {
				if (CAS(&last->next, next, node)) {
#ifdef _VERBOSE
					printf("Enqueue {producerID=%d"
					    " timestamp=%d} succeeded\n",
					    pid, ts);
#endif /* _VERBOSE */
					break;
				}
			} else
				CAS(&q->Tail, last, next);
		}
	}
	CAS(&q->Tail, last, node);
}

struct info *
lfdequeue(struct lfqueue *q)
{
	struct lfqueue_node *first, *last, *next;
	struct info *result;

	result = malloc(sizeof(struct info));
	if (result == NULL) {
		printf("malloc() failed\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		first = q->Head;
		last = q->Tail;
		next = first->next;
		if (first == q->Head) {
			if (first == last) {
				if (next == NULL) {
#ifdef _VERBOSE
					printf("Queue is empty\n");
#endif /* _VERBOSE*/
					free(result);
					return NULL;
				}
				CAS(&q->Tail, last, next);
			} else {
				result->producerID = next->inf.producerID;
				result->timestamp = next->inf.timestamp;
				if (CAS(&q->Head, first, next)) {
#ifdef _VERBOSE
					printf("Dequeue {producerID=%d "
					    "timestamp=%d} succeeded\n",
					    result->producerID,
					    result->timestamp);
#endif /* _VERBOSE*/
					break;
				}
			}
		}
	}
	return result;
}

#ifdef _UTEST

#define NUM_THREADS 3

void *
produce(void *arg)
{
	struct thrdfuncargs *args;
	pthread_t self_tid;
	int self_id, i, timestamp;

	args = (struct thrdfuncargs *)arg;

	self_tid = pthread_self();
	for (i = 0; i < NUM_THREADS; i++) {
		if (self_tid == args->tid[i]) {
			self_id = i;
			break;
		}
	}
	printf("self_id=%d\n", self_id);

	for (i = 0; i <= NUM_THREADS - 1; i++) {
		timestamp = (i * NUM_THREADS) + self_id;
		lfenqueue(args->queue, self_id, timestamp);
	}
}

void *
consume(void *arg)
{
	struct thrdfuncargs *args;
	struct info *result = NULL;

	args = (struct thrdfuncargs *)arg;

	while (1) {
		result = lfdequeue(args->queue);
		if (result == NULL)
			break;
	}
}

int
main()
{
	pthread_t tid[NUM_THREADS];
	struct lfqueue q;
	struct thrdfuncargs args;
	int i, e;

	initlfqueue(&q);

	printf("This is just a test. A queue has been initialized.\n");
	printf("Head={producerID=%d timestamp=%d}\n",
	    q.Head->inf.producerID, q.Head->inf.timestamp);
	printf("Tail={producerID=%d timestamp=%d}\n",
	    q.Tail->inf.producerID, q.Tail->inf.timestamp);	

	args.queue = &q;
	args.tid = tid;
	/* Spawn threads to test concurrent enqueues */
	for (i = 0; i < NUM_THREADS; i++) {
		e = pthread_create(&tid[i], NULL, produce,
		    (void *)&args);
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
	for (i = 0; i < NUM_THREADS; i++) {
		e = pthread_create(&tid[i], NULL, consume,
		    (void *)&args);
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
