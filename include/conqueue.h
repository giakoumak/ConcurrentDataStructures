/*
 * Author: Giannis Giakoumakis
 * Contact: giannis.m.giakoumakis@gmail.com
 *
 * A concurrent unbounded total queue using locks.
 * There is always a sentinel node in this queue,
 * which value is meaningless.
 * Initially both the head and the tail point to the
 * sentinel node.
 */

#ifndef CONQUEUE_H
#define CONQUEUE_H

#include <pthread.h>

#include "common_structs.h"

struct queue {
	struct queue_node *Head;
	struct queue_node *Tail;
	pthread_mutex_t head_lock;
	pthread_mutex_t tail_lock;
};

struct queue_node {
	struct info inf;
	struct queue_node *next;
};

#ifdef _UTEST
struct producer_attr {
	struct queue *queue;
	pthread_t *tid;
};

struct consumer_attr {
	struct queue *queue;
};
#endif /* _UTEST */

/* Initialization of a queue */
void initqueue(struct queue *);

/* Enqueue a new node into a queue */
void enqueue(struct queue *, int, int);

/*
 * Delete a node from a queue.
 * Return the value of the deleted node if
 * the queue is not empty, NULL otherwise.
 */
struct info * dequeue(struct queue *);

#endif /* CONQUEUE_H */
