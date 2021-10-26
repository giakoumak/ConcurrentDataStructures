/*
 * Author: Giannis Giakoumakis
 * Contact: giannis.m.giakoumakis@gmail.com
 *
 * A concurrent binary search tree using fine grain
 * locking. Inorder traversal will give the nodes 
 * of the tree in increasing order. No duplicate keys
 * are allowed. Initially there are 0 nodes in the tree.
 */

#ifndef CONBST_H
#define CONBST_H

#include <pthread.h>

#include "common_structs.h"

struct tree {
	struct tree_node *root;
	pthread_mutex_t tree_lock;
};

struct tree_node {
	struct info inf;
	pthread_mutex_t lock;
	struct tree_node *lc;
	struct tree_node *rc;
};

#ifdef _UTEST
#include "pthread_barrier.h"

struct product {
	struct tree *tree;
	pthread_t *tid;
	pthread_barrier_t barrier;
};
#endif /* _UTEST */

/* Initialization of a BST */
void inittree(struct tree *);

/* In-order print of a BST using recursion */
void print_inorder(struct tree_node *);

/* Insert a new node into a BST */
void insert(struct tree *, int, int);

/*
 * Delete a node from a BST.
 * Return the value of the deleted node if it
 * exists, NULL otherwise.
 */
struct info * delete(struct tree *, int);

/*
 * Helper function to find the node that should
 * be physically deleted from a BST if it
 * exists. Returns NULL to indicate that no
 * such node exists.
 */
struct tree_node * findhelper(struct tree_node *);

#endif /* CONBST_H */
