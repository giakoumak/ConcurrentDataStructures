/*
 * Author: Giannis Giakoumakis
 * Contact: giannis.m.giakoumakis@gmail.com
 */

#include <stdio.h>
#include <stdlib.h>

#include "../include/conbst.h"

void
inittree(struct tree *t)
{
	int e;

	/* Initialization of the tree */
	t->root = NULL;
	e = pthread_mutex_init(&t->tree_lock, NULL);
	if (e != 0) {
		printf("pthread_mutex_init() failed\n");
		exit(EXIT_FAILURE);
	}
}

void
print_inorder(struct tree_node *n)
{
	if (n == NULL)
		return;
	if (n->lc != NULL)
		print_inorder(n->lc);
	printf("producerID=%d timestamp=%d\n",
	    n->inf.producerID, n->inf.timestamp);
	if (n->rc != NULL)
		print_inorder(n->rc);
}

void
insert(struct tree *t, int pid, int ts)
{
	struct tree_node *helper, *curr, *parent;
	int e;

	helper = malloc(sizeof(struct tree_node));
	if (helper == NULL) {
		printf("malloc() failed\n");
		exit(EXIT_FAILURE);
	}

	/* Initialize the fields of the new node */
	helper->inf.producerID = pid;
	helper->inf.timestamp = ts;
	e = pthread_mutex_init(&helper->lock, NULL);
	if (e != 0) {
		printf("pthread_mutex_init() failed\n");
		exit(EXIT_FAILURE);
	}
	helper->lc = NULL;
	helper->rc = NULL;

	pthread_mutex_lock(&t->tree_lock);
	curr = t->root;
	if (curr == NULL) {
	/*
	 * Tree is empty, just set the new allocated node to
	 * be the root of the tree.
	 */
		t->root = helper;
		pthread_mutex_unlock(&t->tree_lock);
#ifdef _VERBOSE
		printf("%d (root) inserted\n", ts);
#endif /* _VERBOSE */
		return;
	}

	/*
	 * Tree is not empty, start searching for the correct
	 * insertion point of the new allocated node.
	 */
	pthread_mutex_lock(&curr->lock);
	pthread_mutex_unlock(&t->tree_lock);
	while (1) {
		parent = curr;
		if (curr->inf.timestamp > ts) /* search left subtree */
			curr = curr->lc;
		else if (curr->inf.timestamp < ts) /*search right subtree */
			curr = curr->rc;
		else { /* found duplicate */
			pthread_mutex_unlock(&curr->lock);
#ifdef _VERBOSE
			printf("Error: %d already in the tree\n", ts);
#endif /* _VERBOSE */
			return;
		}

		if (curr != NULL) {
		/*
		 * Not yet found neither the position of the potential
		 * insertion, nor a duplicate. Continue hand over hand
		 * locking to go deeper into the tree.
		 */
			pthread_mutex_lock(&curr->lock);
			pthread_mutex_unlock(&parent->lock);
		} else /* found the insertion point */
			break;
	}

	if (parent->inf.timestamp > ts)
		parent->lc = helper;
	else
		parent->rc = helper;
	pthread_mutex_unlock(&parent->lock);
#ifdef _VERBOSE
	printf("%d inserted\n", ts);
#endif /* _VERBOSE */
	return;
}

struct info *
delete(struct tree *t, int ts)
{
	struct info *result;
	struct tree_node *helper, *curr, *parent;

	curr = t->root;
	parent = t->root;

	result = malloc(sizeof(struct info));
	if (result == NULL) {
		printf("malloc() failed\n");
		exit(EXIT_FAILURE);
	}

	pthread_mutex_lock(&t->tree_lock);
	if (curr == NULL) { /* tree is empty */
		free(result);
		pthread_mutex_unlock(&t->tree_lock);
#ifdef _VERBOSE
		printf("Error: empty tree\n");
#endif /* _VERBOSE */
		return NULL;
	}

	/* tree is NOT empty, start checking */
	pthread_mutex_lock(&curr->lock);
	if (curr->inf.timestamp > ts) /* search left subtree */
		curr = curr->lc;
	else if (curr->inf.timestamp < ts) /* search right subtree */
		curr = curr->rc;
	else { /* root should be deleted */
		result->producerID = curr->inf.producerID;
		result->timestamp = curr->inf.timestamp;
		helper = findhelper(curr);
		if (helper != NULL) {
			curr->inf.producerID = helper->inf.producerID;
			curr->inf.timestamp = helper->inf.timestamp;
			free(helper);
		} else
			t->root = NULL;
		pthread_mutex_unlock(&curr->lock);
		pthread_mutex_unlock(&t->tree_lock);
#ifdef _VERBOSE
		printf("%d (root) deleted\n", ts);
#endif /* _VERBOSE */
		return result;
	}

	/* should NOT delete the root */
	if (curr != NULL) {
		pthread_mutex_lock(&curr->lock);
		pthread_mutex_unlock(&t->tree_lock);
	} else {
		free(result);
		pthread_mutex_unlock(&t->tree_lock);
		pthread_mutex_unlock(&parent->lock);
#ifdef _VERBOSE
		printf("Error: %d does not exist\n", ts);
#endif /* _VERBOSE */
		return NULL;
	}

	/*
	 * Start searching deeper considering the fact that the
	 * corresponding (to the ts) node may not exist at all.
	 */
	while (1) {
		if (curr->inf.timestamp > ts) {
		/* search left subtree */
			pthread_mutex_unlock(&parent->lock);
			parent = curr;
			curr = curr->lc;
		} else if (curr->inf.timestamp < ts) {
		/* search right subtree */
			pthread_mutex_unlock(&parent->lock);
			parent = curr;
			curr = curr->rc;
		} else {
		/* found the node that should be deleted */
			result->producerID = curr->inf.producerID;
			result->timestamp = curr->inf.timestamp;
			helper = findhelper(curr);
			if (helper != NULL) {
				curr->inf.producerID = helper->inf.producerID;
				curr->inf.timestamp = helper->inf.timestamp;
				free(helper);
			} else {
				if (parent->lc == curr)
					parent->lc = NULL;
				else
					parent->rc = NULL;
			}
			pthread_mutex_unlock(&curr->lock);
			pthread_mutex_unlock(&parent->lock);
#ifdef _VERBOSE
			printf("%d deleted\n", ts);
#endif /* _VERBOSE */
			return result;
		}

		if (curr == NULL) {
		/*
		 * Cannot go any deeper and no corresponding node
		 * had been found, so unlock the parent and return
		 * NULL pointer to indicate that no deletion took
		 * place.
		 */
			free(result);
			pthread_mutex_unlock(&parent->lock);
#ifdef _VERBOSE
			printf("Error: %d does not exist\n", ts);
#endif /* _VERBOSE */
			return NULL;
		}

		/*
		 * Lock the current node and repeat until search either
		 * succeeds or it fails.
		 */
		pthread_mutex_lock(&curr->lock);
	}
}

struct tree_node *
findhelper(struct tree_node *n)
{
	struct tree_node *curr, *parent;

	/*
	 * No children exist, node will be physically deleted from the
	 * tree, no value replacement will happen.
	 */
	if (n->lc == NULL && n->rc == NULL)
		return NULL;
	
	if (n->lc != NULL) {
	/*
	 * Left child does exist so find the previous node (predecessor)
	 * to n (based on in-order traversal) and return it.
	 * Before the return, appropriate actions take place, in order to
	 * be able to physically disconnect the predecessor from the tree
	 * without affecting the BST invariants. 
	 */
		parent = n;
		curr = n->lc;
		pthread_mutex_lock(&curr->lock);
		while(curr->rc != NULL) {
			if (parent != n)
				pthread_mutex_unlock(&parent->lock);
			parent = curr;
			curr = curr->rc;
			pthread_mutex_lock(&curr->lock);
		}
		if (curr->lc != NULL)
			pthread_mutex_lock(&curr->lc->lock);
		if (parent == n)
			parent->lc = curr->lc;
		else {
			parent->rc = curr->lc;
			pthread_mutex_unlock(&parent->lock);
		}
		if (curr->lc != NULL)
			pthread_mutex_unlock(&curr->lc->lock);
		pthread_mutex_unlock(&curr->lock);
		return curr;
	} else {
	/*
	 * Left child does NOT exist but right child DOES so find the
	 * next node (successor) to n (based on in-order traversal) and
	 * return it.
	 * Before the return, appropriate actions take place, in order to
	 * be able to physically disconnect the successor from the tree
	 * without affecting the BST invariants. 
	 */
		parent = n;
		curr = n->rc;
		pthread_mutex_lock(&curr->lock);
		while(curr->lc != NULL) {
			if (parent != n)
				pthread_mutex_unlock(&parent->lock);
			parent = curr;
			curr = curr->lc;
			pthread_mutex_lock(&curr->lock);
		}
		if (curr->rc != NULL)
			pthread_mutex_lock(&curr->rc->lock);
		if (parent == n)
			parent->rc = curr->rc;
		else {
			parent->lc = curr->rc;
			pthread_mutex_unlock(&parent->lock);
		}
		if (curr->rc != NULL)
			pthread_mutex_unlock(&curr->rc->lock);
		pthread_mutex_unlock(&curr->lock);
		return curr;
	}
}

#ifdef _UTEST

#define NUM_THREADS 2

void *
produce_consume(void *arg)
{
	struct product *prod;
	struct info *result;
	pthread_t self_tid;
	int self_id, i, timestamp;

	prod = (struct product *)arg;
	
	self_tid = pthread_self();
	for (i = 0; i < NUM_THREADS; i++) {
		if (self_tid == prod->tid[i]) {
			self_id = i;
			break;
		}
	}
	printf("self_id=%d\n", self_id);

	for (i = 0; i <= NUM_THREADS - 1; i++) {
		timestamp = (i * NUM_THREADS) + self_id;
		insert(prod->tree, self_id, timestamp);
	}

	pthread_barrier_wait(&prod->barrier);
	
	i = 0;	
	while (1) {
		timestamp = ((i * NUM_THREADS) + self_id);
		result = delete(prod->tree, timestamp);
		i++;
		if (result == NULL)
			break;
	}
}

int
main()
{
/*
 * Visual representation of the tree after the insertions:
 * Unit test #1 (serial execution)
 *
 *       10
 *      /  \
 *     8   12
 *    /   /  \
 *   5   11  19
 *  / \     /  \
 * 2   7   17  25
 *
 */
	pthread_t tid[NUM_THREADS];
	pthread_barrier_t barrier;
	struct tree t;
	struct product prod;
	int i, e;

	inittree(&t);

	printf("This is just a test. A tree has been initialized.\n");

	printf("Unit test #1 (serial execution)\n");
	insert(&t, 10, 10);
	insert(&t, 8, 8);
	insert(&t, 12, 12);
	insert(&t, 5, 5);
	insert(&t, 19, 19);
	insert(&t, 11, 11);
	insert(&t, 25, 25);
	insert(&t, 7, 7);
	insert(&t, 2, 2);
	insert(&t, 5, 5); /* try to insert duplicate */
	insert(&t, 17, 17);
	print_inorder(t.root);
	delete(&t, 50); /* try to delete nonexistent key */
	delete(&t, 19);
	delete(&t, 15); /* try to delete nonexistent key */
	delete(&t, 8);
	delete(&t, 10);
	delete(&t, 25);
	delete(&t, 12);
	delete(&t, 5);
	delete(&t, 7);
	delete(&t, 17);
	delete(&t, 2);
	delete(&t, 11);
	print_inorder(t.root);

	/*
	 * Spawn threads to test concurrent insertions and
	 * deletions. Sync those functionalities using a
	 * barrier to ensure that all insertions took place
	 * before the first deletion.
	 */
	printf("Unit test #2 (concurrent execution)\n");
	e = pthread_barrier_init(&prod.barrier, NULL, NUM_THREADS);
	if (e != 0) {
		printf("pthread_barrier_init() failed\n");
		exit(EXIT_FAILURE);
	}
	prod.tree = &t;
	prod.tid = tid;

	for (i = 0; i < NUM_THREADS; i++) {
		e = pthread_create(&tid[i], NULL, produce_consume,
		    (void *)&prod);
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
	print_inorder(t.root);

	return 0;
}

#endif /* _UTEST */
