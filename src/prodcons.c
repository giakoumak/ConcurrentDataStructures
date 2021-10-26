/*
 * Author: Giannis Giakoumakis
 * Contact: giannis.m.giakoumakis@gmail.com
 */

#include <stdlib.h>
#include <stdio.h>

#include "../include/prodcons.h"
#include "../include/pthread_barrier.h"

int
main(int argc, char **argv)
{
	pthread_t *producers, *consumers;
	pthread_barrier_t barrier;
#if defined _LOCK_FREE_QUEUE
	struct lfqueue q;
#else
	struct queue q;
#endif
	struct tree t;
	struct producerinfo pinfo;
	struct consumerinfo cinfo;
	int nthreads;
	int e, i;

	/* check args */
	if (argc != 2)
		usage(EXIT_FAILURE);
	nthreads = atoi(argv[1]);
	if (nthreads <= 0)
		usage(EXIT_FAILURE);

	/*
	 * Allocate memory, initialize data structures, common
	 * barrier and initialize producer/consumer structs.
	 */
	producers = malloc(nthreads * sizeof(pthread_t));
	if (producers == NULL) {
		printf("malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	consumers = malloc(nthreads * sizeof(pthread_t));
	if (consumers == NULL) {
		printf("malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	e = pthread_barrier_init(&barrier, NULL, nthreads * 2);
	if (e != 0) {
		printf("pthread_barrier_init() failed\n");
		exit(EXIT_FAILURE);
	}
#if defined _LOCK_FREE_QUEUE
	initlfqueue(&q);
#else
	initqueue(&q);
#endif
	inittree(&t);

	pinfo.tid = producers;
	pinfo.nthreads = nthreads;
	pinfo.barrier = &barrier;
	pinfo.queue = &q;
	pinfo.tree = &t;

	cinfo.tid = consumers;
	cinfo.nthreads = nthreads;
	cinfo.barrier = &barrier;
	cinfo.tree = &t;

	/* Spawn producers */
	for (i = 0; i < nthreads; i++) {
		e = pthread_create(&producers[i], NULL, produce,
		    (void *)&pinfo);
		if (e != 0) {
			printf("pthread_create() failed\n");
			exit(EXIT_FAILURE);
		}
	}
	/* Spawn consumers */
	for (i = 0; i < nthreads; i++) {
		e = pthread_create(&consumers[i], NULL, consume,
		    (void *)&cinfo);
		if (e != 0) {
			printf("pthread_create() failed\n");
			exit(EXIT_FAILURE);
		}
	}
	/*
	 * Join every spawned thread, both the consumers and
	 * the producers
	 */
	for (i = 0; i < nthreads; i++) {
		e = pthread_join(producers[i], NULL);
		if (e != 0) {
			printf("pthread_join() failed\n");
			exit(EXIT_FAILURE);
		}
		e = pthread_join(consumers[i], NULL);
		if (e != 0) {
			printf("pthread_join() failed\n");
			exit(EXIT_FAILURE);
		}
	}

	return 0;
}

void
usage(int exit_code)
{
	printf("usage: ./prodcons.x N\n"
	    "\tN: number of producers/consumers\n");
	exit(exit_code);
}

void *
produce(void *arg)
{
	struct producerinfo *pinfo;
	pthread_t tid; /* threadID */
	int pid; /* producerID */
	struct info *result;
	int i, timestamp;

	pinfo = (struct producerinfo *)arg;

	/* Map pthread_t to int to be more readable */
	tid = pthread_self();
	for (i = 0; i < pinfo->nthreads; i++) {
		if (tid == pinfo->tid[i]) {
			pid = i;
			break;
		}
	}

	/* Data production phase using a shared queue */
	printf("producer%d just start inserting into the"
	    " shared queue\n", pid);
	for (i = 0; i < pinfo->nthreads; i++) {
		timestamp = (i * pinfo->nthreads) + pid;
#if defined _LOCK_FREE_QUEUE
		lfenqueue(pinfo->queue, pid, timestamp);
#else
		enqueue(pinfo->queue, pid, timestamp);
#endif
	}

	/*
	 * Make sure that every producer has enqueued his
	 * data into the shared queue before the announcement
	 * of the data to the consumers take place.
	 */
	pthread_barrier_wait(pinfo->barrier);

	/* Data announcement phase using a shared tree */
	printf("producer%d just start removing from the"
	    " shared queue and inserting into the shared"
	    " binary search tree\n", pid);
	while (1) {
#if defined _LOCK_FREE_QUEUE
		result = lfdequeue(pinfo->queue);
#else
		result = dequeue(pinfo->queue);
#endif
		if (result == NULL)
			break;
		insert(pinfo->tree, result->producerID,
		    result->timestamp);
	}

	return NULL;
}

void *
consume(void *arg)
{
	struct consumerinfo *cinfo;
	struct info *result;
	pthread_t tid; /* threadID */
	int pid; /* producerID */
	int cid; /* consumerID */
	int i, timestamp;

	cinfo = (struct consumerinfo *)arg;

	/* Map pthread_t to int to be more readable */
	tid = pthread_self();
	for (i = 0; i < cinfo->nthreads; i++) {
		if (tid == cinfo->tid[i]) {
			cid = i;
			break;
		}
	}
	/*
	 * Each consumer should consume data produced by
	 * a specific producer based on his ID.
	 * So using a modulo function, map a the consumer
	 * to his corresponding producer.
	 */
	pid = modulo(cid - 1, cinfo->nthreads);

	/*
	 * Same barrier used by the producers. It ensures that
	 * the consumers will start consuming data after the
	 * publication has been started by the producers.
	 * In other words, data will be announced by the
	 * producers to the consumers and being consumed
	 * concurrently.
	 */
	pthread_barrier_wait(cinfo->barrier);

	/* Data consuming using a shared tree */
	printf("consumer%d just start removing from the"
	    " shared binary search tree\n", cid);
	i = 0;
	while (i < cinfo->nthreads) {
		timestamp = ((i * cinfo->nthreads) + pid);
		result = delete(cinfo->tree, timestamp);
		if (result != NULL) {
			printf("consumerID=%d consumed timestamp=%d"
			    " produced by producerID=%d\n", cid,
			    result->timestamp, result->producerID);
			i++;
		}
	}
	printf("consumerID=%d consumed %d chunks of data\n",
	    cid, i);

	return NULL;
}

int
modulo(int x, int y)
{
	return (x % y + y) % y;
}
