/*
 * addrtree -- radix tree for vandergaast cache.
 *
 * Copyright (c) 2013, NLnet Labs.  See LICENSE for license.
 */

/** 
 * \file
 * The addrtree is a radix tree designed for vandergaast. Most notable
 * is the addition of 'scope' to a node. Scope is only relevant for 
 * nodes with elem set, it indicates the number of bits the authority
 * desires.
 * 
 * For retreiving data one needs an address and address length 
 * (sourcemask). While traversing the tree the first matching node is 
 * returned. A node matches when 
 * 		node.scope<=sourcemask && node.elem!=NULL 
 * 		(This is the most specific answer the authority has.)
 * or
 * 		node.sourcemask==sourcemask && node.elem!=NULL
 * 		(This is the most specific question the client can ask.)
 * 
 * Insertion needs an address, sourcemask and scope. The length of the 
 * address is capped by min(sourcemask, scope). While traversing the 
 * tree the scope of all visited nodes is updated. This ensures we are 
 * always able to find the most specific answer available.
 * 
 * At this time the tree does not support deletion of nodes, but elem 
 * can be set to NULL to ignore the node.
 */

#ifndef ADDRTREE_H
#define ADDRTREE_H

typedef uint8_t addrlen_t;
typedef uint8_t addrkey_t;
#define KEYWIDTH 8

struct addrtree {
	struct addrnode* root;
	/** Number of elements in the tree (not always equal to number of 
	 * nodes) */
	unsigned int elem_count;
	/** Maximum prefix length we are willing to cache. */
	addrlen_t max_depth;
	/** External function to delete elem. Called as 
	 * delfunc(addrnode->elem, addrtree->env) */
	void (*delfunc)(void *, void *);
	/** Environment for delfunc */
	void *env;
	/** External function returning size of elem. Called as
	 * sizefunc(addrnode->elem) */
	size_t (*sizefunc)(void *);
};

struct addrnode {
	/** Payload of node, may be NULL */
	void* elem;
	/** Abs time in seconds in which elem is meaningful */
	time_t ttl;
	/** Number of significant bits in address. */
	addrlen_t scope;
	/** A node can have 0-2 edges, set to NULL for unused */
	struct addredge* edge[2];
	/** edge between this node and parent */
	struct addredge* parent_edge;
};

struct addredge {
	/** address of connected node */
	addrkey_t* str;
	/** lenght in bits of str */
	addrlen_t len;
	/** child node this edge is connected to */
	struct addrnode* node;
	/** Parent node this ege is connected to */
	struct addrnode* parent_node;
	/** Index of this edge in parent_node */
	int parent_index;
};

/**
 *  Size of tree in bytes
 */
size_t addrtree_size(const struct addrtree* tree);

/** 
 * Create a new tree
 * @param max_depth: Tree will cap keys to this length.
 * @param delfunc: f(element, env) delete element
 * @param sizefunc: f(element) returning the size of element.
 * @param env: Module environment for alloc information
 * @return new addrtree or NULL on failure
 */
struct addrtree* 
addrtree_create(addrlen_t max_depth, void (*delfunc)(void *, void *), 
	size_t (*sizefunc)(void *), void *env);

/** 
 * Free tree and all nodes below
 * @param tree: Tree to be freed
 */
void addrtree_delete(struct addrtree* tree);

/**
 * Insert an element in the tree. Failures are silent. Sourcemask and
 * scope might be changed according to local policy. Caller should no 
 * longer access elem, it could be free'd now or later during future
 * inserts.
 * 
 * @param tree: Tree insert elem in
 * @param addr: key for element lookup
 * @param sourcemask: Length of addr in bits
 * @param scope: Number of significant bits in addr
 * @param elem: data to store in the tree
 * @param ttl: elem is valid up to this time, seconds.
 * @param now: Current time in seconds
 */
void addrtree_insert(struct addrtree* tree, const addrkey_t* addr, 
	addrlen_t sourcemask, addrlen_t scope, void* elem, time_t ttl, 
	time_t now);

/**
 * Find a node containing an element in the tree.
 * 
 * @param tree: Tree to search
 * @param addr: key for element lookup
 * @param sourcemask: Length of addr in bits
 * @param now: Current time in seconds
 * @return addrnode or NULL on miss
 */
struct addrnode* addrtree_find(const struct addrtree* tree, 
	const addrkey_t* addr, addrlen_t sourcemask, time_t now);

/** Wrappers for static functions to unit test */
int unittest_wrapper_addrtree_cmpbit(const addrkey_t* key1, 
	const addrkey_t* key2, addrlen_t n);
addrlen_t unittest_wrapper_addrtree_bits_common(const addrkey_t* s1, 
	addrlen_t l1, const addrkey_t* s2, addrlen_t l2, addrlen_t skip);
int unittest_wrapper_addrtree_getbit(const addrkey_t* addr, 
	addrlen_t addrlen, addrlen_t n);
int unittest_wrapper_addrtree_issub(const addrkey_t* s1, addrlen_t l1, 
	const addrkey_t* s2, addrlen_t l2,  addrlen_t skip);
#endif /* ADDRTREE_H */
