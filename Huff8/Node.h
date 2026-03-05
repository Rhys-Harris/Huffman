#ifndef _NODE_H_
#define _NODE_H_

#include <stdbool.h>

#include "./WriteNode.h"

#define MAX_NODES 128
#define MAX_NODE_DEPTH 20

typedef struct Node Node;

typedef struct Node {
		char symbol;
		int count;
		Node *parent;
		Node *left;
		Node *right;
		bool isRight;
} Node;

void destroyNode(Node *node);

int countNodes(Node *node);

void fixParents(Node *node);

bool findPathForSymbol(Node *nodePath, Node *node, int *pathLen, char symbol);

void placeNodeInList(Node *node, WriteNode *nodeList, int numNodes, int *curNodeIndex, int parent);

#endif
