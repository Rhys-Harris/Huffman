#include <stdlib.h>

#include "Node.h"

void destroyNode(Node *node) {
	if (node->left != NULL) {
		destroyNode(node->left);
		free(node->left);
	}

	if (node->right != NULL) {
		destroyNode(node->right);
		free(node->right);
	}
}

int countNodes(Node *node) {
	int count = 1;

	if (node->left != NULL) {
		count += countNodes(node->left);
	}

	if (node->right != NULL) {
		count += countNodes(node->right);
	}

	return count;
}

void fixParents(Node *node) {
	if (node->left != NULL) {
		fixParents(node->left);
		node->left->parent = node;
		node->left->isRight = false;
	}

	if (node->right != NULL) {
		fixParents(node->right);
		node->right->parent = node;
		node->left->isRight = true;
	}
}

bool findPathForSymbol(Node *nodePath, Node *node, int *pathLen, char symbol) {
	// Add this node to the stack
	nodePath[*pathLen] = *node;
	++(*pathLen);

	// Check left
	if (node->left != NULL) {
		bool found = findPathForSymbol(nodePath, node->left, pathLen, symbol);
		if (found) {
			return true;
		}
	}

	// Check right
	if (node->right != NULL) {
		bool found = findPathForSymbol(nodePath, node->right, pathLen, symbol);
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


void placeNodeInList(Node *node, WriteNode *nodeList, int numNodes, int *curNodeIndex, int parent) {
	// Place this node in the list
	nodeList[*curNodeIndex] = (WriteNode){node->symbol, parent, node->isRight};

	const int thisIndex = *curNodeIndex;

	if (node->left != NULL) {
		// Go to the next node in list
		(*curNodeIndex)++;

		// Place left side in
		placeNodeInList(node->left, nodeList, numNodes, curNodeIndex, thisIndex);
	}

	if (node->right != NULL) {
		// Go to the next node in list
		(*curNodeIndex)++;

		// Place right side in
		placeNodeInList(node->right, nodeList, numNodes, curNodeIndex, thisIndex);
	}
}
