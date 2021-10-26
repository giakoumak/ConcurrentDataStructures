/*
 * Author: Giannis Giakoumakis
 * Contact: giannis.m.giakoumakis@gmail.com
 *
 * Common structs that are used in multiple
 * files and for general purposes.
 */

#ifndef COMMON_STRUCTS_H
#define COMMON_STRUCTS_H

/*
 * Data struct used both by the concurrent
 * queue and the concurrent binary search
 * tree.
 */
struct info {
	int producerID;
	int timestamp;
};

#endif /* COMMON_STRUCTS_H */
