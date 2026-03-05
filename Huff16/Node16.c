#include <stdlib.h>
#include <stdio.h>

#include "Node16.h"

void destroyNode16(Node16 *node) {
	if (node->left != NULL) {
		destroyNode16(node->left);
		free(node->left);
	}

	if (node->right != NULL) {
		destroyNode16(node->right);
		free(node->right);
	}
}

int countNode16s(Node16 *node) {
	int count = 1;

	if (node->left != NULL) {
		count += countNode16s(node->left);
	}

	if (node->right != NULL) {
		count += countNode16s(node->right);
	}

	return count;
}

void fixParents16(Node16 *node) {
	if (node->left != NULL) {
		fixParents16(node->left);
		node->left->parent = node;
		node->left->isRight = false;
	}

	if (node->right != NULL) {
		fixParents16(node->right);
		node->right->parent = node;
		node->left->isRight = true;
	}
}

bool findPathForSymbol16(Node16 *nodePath, Node16 *node, int *pathLen, short symbol) {
	if (*pathLen == MAX_NODE_DEPTH) {
		printf("Ran out of path space\n");
		return false;
	}

	// Add this node to the stack
	nodePath[*pathLen] = *node;
	++(*pathLen);

	// Check left
	if (node->left != NULL) {
		bool found = findPathForSymbol16(nodePath, node->left, pathLen, symbol);
		if (found) {
			return true;
		}
	}

	// Check right
	if (node->right != NULL) {
		bool found = findPathForSymbol16(nodePath, node->right, pathLen, symbol);
		if (found) {
			return true;
		}
	}

	// Check self
	if (node->symbol == symbol) {
		return true;
	}

	// Pop off the stack
	--(*pathLen);
	
	// Return a fail
	return false;
}

void placeNode16InList(Node16 *node, WriteNode16 *nodeList, int numNodes, int *curNodeIndex, int parent) {
	// Place this node in the list
	nodeList[*curNodeIndex] = (WriteNode16){node->symbol, parent, node->isRight};

	const int thisIndex = *curNodeIndex;

	if (node->left != NULL) {
		// Go to the next node in list
		(*curNodeIndex)++;

		// Place left side in
		placeNode16InList(node->left, nodeList, numNodes, curNodeIndex, thisIndex);
	}

	if (node->right != NULL) {
		// Go to the next node in list
		(*curNodeIndex)++;

		// Place right side in
		placeNode16InList(node->right, nodeList, numNodes, curNodeIndex, thisIndex);
	}
}
