/*
 * Author: Giannis Giakoumakis
 * Contact: giannis.m.giakoumakis@gmail.com
 */

#ifndef PRODCONS_H
#define PRODCONS_H

#include <pthread.h>

#if defined _LOCK_FREE_QUEUE
#include "conlfqueue.h"
#else
#include "conqueue.h"
#endif

#include "conbst.h"
#include "pthread_barrier.h"

struct producerinfo {
	pthread_t *tid;
	int nthreads;
	pthread_barrier_t *barrier;
#if defined _LOCK_FREE_QUEUE
	struct lfqueue *queue;
#else
	struct queue *queue;
#endif
	struct tree *tree;
};

struct consumerinfo {
	pthread_t *tid;
	int nthreads;
	pthread_barrier_t *barrier;

	struct tree *tree;
};

void usage(int);

void * produce(void *);

void * consume(void *);

int modulo(int, int);

#endif /* PRODCONS_H */
