#ifndef _NODE_32_H_
#define _NODE_32_H_

#include <stdbool.h>

#include "WriteNode32.h"

// NOTE: This max nodes thing is kind of screwy
#define MAX_NODES 5000
#define MAX_NODE_DEPTH 100

typedef struct Node32 Node32;

typedef struct Node32 {
		unsigned int symbol;
		int count;
		Node32 *parent;
		Node32 *left;
		Node32 *right;
		bool isRight;
} Node32;

void destroyNode32(Node32 *node);

int countNode32s(Node32 *node);

void fixParents32(Node32 *node);

bool findPathForSymbol32(Node32 *nodePath, Node32 *node, int *pathLen, unsigned int symbol);

void placeNode32InList(Node32 *node, WriteNode32 *nodeList, int numNode32s, int *curNode32Index, int parent);

#endif
