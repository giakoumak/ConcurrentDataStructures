
/*
 * Author: Giannis Giakoumakis
 * Contact: giannis.m.giakoumakis@gmail.com
 *
 * A concurrent unbounded lock free queue.
 * The first node in the queue is a sentinel node,
 * whose value is meaningless.
 * Threads may need to help each other to ensure
 * lock-freedom.
 */

#ifndef CONLFQUEUE_H
#define CONLFQUEUE_H

#include <pthread.h>

#include "common_structs.h"

struct lfqueue {
	struct lfqueue_node *Head;
	struct lfqueue_node *Tail;
};

struct lfqueue_node {
	struct info inf;
	struct lfqueue_node *next;
};

#ifdef _UTEST
struct thrdfuncargs {
	struct lfqueue *queue;
	pthread_t *tid;
};
#endif /* _UTEST */

/* Initialization of a lock free queue */
void initlfqueue(struct lfqueue *);

/* Enqueue a new node into a lock free queue */
void lfenqueue(struct lfqueue *, int, int);

/*
 * Delete a node from a lock free queue.
 * Return the value of the deleted node if
 * the queue is not empty, NULL otherwise.
 */
struct info * lfdequeue(struct lfqueue *);

#endif /* CONLFQUEUE_H */

