#ifndef _NODE_16_H_
#define _NODE_16_H_

#include <stdbool.h>

#include "WriteNode16.h"

#define MAX_NODES 500
#define MAX_NODE_DEPTH 20

typedef struct Node16 Node16;

typedef struct Node16 {
		short symbol;
		int count;
		Node16 *parent;
		Node16 *left;
		Node16 *right;
		bool isRight;
} Node16;

void destroyNode16(Node16 *node);

int countNode16s(Node16 *node);

void fixParents16(Node16 *node);

bool findPathForSymbol16(Node16 *nodePath, Node16 *node, int *pathLen, short symbol);

void placeNode16InList(Node16 *node, WriteNode16 *nodeList, int numNode16s, int *curNode16Index, int parent);

#endif
