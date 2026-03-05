#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "HuffEntry.h"
#include "Node.h"
#include "../BufView32.h"

#include "HuffCompress.h"

#define OUT_TEXT_MAX_SIZE 100000

#define MAX_CHARS 100000

// Returns the complete table, giving count for each char
HuffEntry *getUniqueSymbols(const char *text, int *numSymbols) {
	// Used to count each char
	int staticTable[256];

	// Start all counts at 0
	memset(staticTable, 0, 256*sizeof(int));

	// Count each symbol
	for (int i = 0; text[i] != 0; ++i) {
		staticTable[text[i]]++;
	}

	// Count how many uniques there are
	int uniques = 0;
	for (int i = 0; i < 256; ++i) {
		if (staticTable[i] != 0) {
			++uniques;
		}
	}
	*numSymbols = uniques;
	printf("Found %i unique symbols\n", uniques);

	// Create entry table
	HuffEntry *entries = malloc(uniques*sizeof(HuffEntry));
	if (entries == NULL) {
		printf("Couldn't allocate entry table");
		return NULL;
	}

	// Place counts into table
	int tableIndex = 0;
	for (int i = 0; i < 256; ++i) {
		if (staticTable[i] == 0) {
			continue;
		}

		entries[tableIndex].symbol = (char)i;
		entries[tableIndex].count = staticTable[i];
		++tableIndex;
	}

	return entries;
}

void sortEntries(HuffEntry *entries, const int numSymbols) {
	for (int i = 0; i < numSymbols-1; ++i) {
		for (int j = 1; j < numSymbols-i; ++j) {
			if (entries[j-1].count < entries[j].count) {
				const HuffEntry temp = entries[j-1];
				entries[j-1] = entries[j];
				entries[j] = temp;
			}
		}
	}
}

errno_t createHuffmanTree(Node nodes[MAX_NODES], const HuffEntry *entries, const int numSymbols, int *outNumNodes) {
	int numNodes = 0;

	for (int i = 0; i < numSymbols; ++i) {
		nodes[i] = (Node){entries[i].symbol, entries[i].count, NULL, NULL, NULL, false};
	}

	numNodes = numSymbols;

	int i = numNodes-1;

	// While there are at least 2 root nodes
	printf("Creating huffman tree\n");
	while (i >= 1) {
		Node nodeA = nodes[i-1];
		Node nodeB = nodes[i];

		// Create a new parent node
		Node parent = (Node){0, nodeA.count + nodeB.count, NULL, NULL, NULL, false};

		// Allocate space for each child node
		parent.left = malloc(sizeof(Node));
		if (parent.left == NULL) {
			printf("Couldn't allocate memory\n");
			return 1;
		}

		parent.right = malloc(sizeof(Node));
		if (parent.right == NULL) {
			printf("Couldn't allocate memory\n");
			return 1;
		}

		// Place in the children
		*parent.left = nodeA;
		*parent.right = nodeB;

		// Take the nodes out of the array
		numNodes -= 2;

		// Find index to place new parent
		int j = 0;
		while (j < numNodes) {
			if (nodes[j].count < parent.count) {
				break;
			}
			++j;
		}
		
		// Move everything over
		for (int k = numNodes-1; k >= j; --k) {
			nodes[k+1] = nodes[k];
		}

		// Place parent in
		nodes[j] = parent;
		
		// Increase length to include new node
		++numNodes;

		--i;
	}

	*outNumNodes = numNodes;

	return 0;
}

typedef struct CompStream {
	char *text;
	int nextByteIndex;
	int nextBitIndex;
	int length;
} CompStream;

#define EMPTY_COMP_STREAM (CompStream){NULL, 0, 0}

CompStream createCompressedText(const char *text, Node *root) {
	CompStream out = EMPTY_COMP_STREAM;
	out.text = calloc(OUT_TEXT_MAX_SIZE, sizeof(char));
	if (out.text == NULL) {
		printf("Couldn't allocate memory\n");
		return out;
	}

	Node nodePath[MAX_NODE_DEPTH];
	int pathLen = 0;

	for (int i = 0; text[i] != 0; ++i) {
		bool found = findPathForSymbol(nodePath, root, &pathLen, text[i]);

		if (!found) {
			printf("Couldn't find symbol '%c' ('%i')\n", text[i], text[i]);
			free(out.text);
			return EMPTY_COMP_STREAM;
		}

		for (int j = 1; j < pathLen; ++j) {
			unsigned char byte = 0;
			Node curNode = nodePath[j];

			if (curNode.isRight) {
				byte = 1;
			}

			byte <<= (7-out.nextBitIndex);
			out.text[out.nextByteIndex] |= byte;

			++out.nextBitIndex;
			if (out.nextBitIndex == 8) {
				out.nextBitIndex = 0;
				out.nextByteIndex++;

				if (out.nextByteIndex == OUT_TEXT_MAX_SIZE) {
					printf("Ran out of output space\n");
					return EMPTY_COMP_STREAM;
				}
			}
		}

		pathLen = 0;
	}

	// Calculate the length of the output string
	out.length = out.nextByteIndex + 1 - (out.nextBitIndex==0);

	return out;
}

WriteNode *createWriteTable(Node *root, const int numNodes) {
	WriteNode *nodeList = malloc(numNodes*sizeof(WriteNode));
	int curNodeIndex = 0;

	placeNodeInList(root, nodeList, numNodes, &curNodeIndex, -1);

	return nodeList;
}

errno_t writeAllDataToBuffer(WriteNode *nodeList, const int numNodes, CompStream stream, char **outOut, int *outLen) {
	// Write to a file
	const int bytesNeeded =
		4 + // Number of nodes
		CHARS_PER_WRITE_NODE * numNodes + // Table
		stream.length + // Actual text
		4 + // Last byte index
		4; // Last bit index
	// Length of text will be last byte index + 1

	int lastByteIndex = stream.nextByteIndex;
	int lastBitIndex = stream.nextBitIndex-1;
	if (lastBitIndex == -1) {
		lastBitIndex = 7;
		--lastByteIndex;
	}

	// Confirm just in case
	if (lastByteIndex+1 != stream.length) {
		printf("Last byte index + 1 wasn't stream length\n");
		return 1;
	}

	printf("Output bytes needed: %i\n", bytesNeeded);

	printf("Allocating final string\n");
	char *out = malloc(bytesNeeded);

	// Write metadata
	printf("Writing metadata\n");
	writeIntToBuff(numNodes, 0, (unsigned char*)out);
	writeIntToBuff(lastByteIndex, 4, (unsigned char*)out);
	writeIntToBuff(lastBitIndex, 8, (unsigned char*)out);

	// Write table
	printf("Writing table\n");
	for (int i = 0; i < numNodes; ++i) {
		const int buffIndex = 12 + i * CHARS_PER_WRITE_NODE;
		// const int buff32Index = buffIndex / 4;

		WriteNode node = nodeList[i];
		
		// 0 -> 3
		// outView32[buff32Index] = node.parent;
		writeIntToBuff(node.parent, buffIndex, (unsigned char*)out);

		// 4
		out[buffIndex+4] = (char)node.symbol;

		// 5
		out[buffIndex+5] = (char)node.isRight;
	}

	// Write compressed text
	printf("Writing compressed text\n");
	const int textStartIndex = 12 + CHARS_PER_WRITE_NODE * numNodes;
	for (int i = 0; i <= lastByteIndex; ++i) {
		out[textStartIndex+i] = stream.text[i];
	}

	*outOut = out;
	*outLen = bytesNeeded;

	return 0;
}

// Compresses using huffman and writes directly to the file
errno_t huffCompressFile(const char *infilename, const char *outfilename) {
	// Read in text
	FILE *f;
	if (fopen_s(&f, infilename, "rb")) {
		printf("Couldn't open input file\n");
		return 1;
	}

	char text[MAX_CHARS];
	memset(text, 0, MAX_CHARS);

	fread(text, sizeof(char), MAX_CHARS, f);

	fclose(f);

	return huffCompress(text, outfilename);
}

// Compresses using huffman and writes directly to the file
errno_t huffCompress(const char *text, const char *outfilename) {
	// Count each character
	int numSymbols;
	HuffEntry *entries = getUniqueSymbols(text, &numSymbols);
	if (entries == NULL) {
		printf("Couldn't get unique symbols\n");
		return 1;
	}

	// Sort entries
	sortEntries(entries, numSymbols);

	Node nodes[MAX_NODES];
	int numNodes = 0;

	errno_t err = createHuffmanTree(nodes, entries, numSymbols, &numNodes);
	if (err) {
		printf("Couldn't create huffman tree\n");
		return 1;
	}

	// Destroy symbol counts
	printf("Destroying symbol counts\n");
	free(entries);

	// Keep track of the root node
	printf("Picking root node\n");
	Node root = nodes[0];

	// Get the number of nodes
	numNodes = countNodes(&root);
	printf("Tree made of %i nodes\n", numNodes);

	// Fix parents
	printf("Fixing parents\n");
	fixParents(&root);

	// Use the tree to compress the data
	CompStream stream = createCompressedText(text, &root);
	if (stream.text == NULL) {
		printf("Couldn't compress text\n");
		return 1;
	}

	printf("Compressed Text Length: %i\n", stream.length);

	// Compress tree to a writable table
	WriteNode *nodeList = createWriteTable(&root, numNodes);

	// Destroy the huffman tree
	printf("Destroying huffman tree\n");
	destroyNode(&root);

	char *out;
	int outLen;
	err = writeAllDataToBuffer(nodeList, numNodes, stream, &out, &outLen);
	if (err) {
		printf("Couldn't write data to buffer\n");
		return 1;
	}

	// Destroy node list
	printf("Destroying write node list\n");
	free(nodeList);

	// Destroy comp text buffer
	printf("Destroying comp text buffer\n");
	free(stream.text);

	printf("Writing...\n");

	FILE *f;
	err = fopen_s(&f, outfilename, "wb");
	if (err) {
		printf("Couldn't open output file\n");
		return 1;
	}
	fwrite(out, sizeof(char), outLen, f);
	fclose(f);

	printf("Done write\n\n");

	// Destroy final buffer
	printf("Destroying output buffer\n");
	free(out);

	return 0;
}
