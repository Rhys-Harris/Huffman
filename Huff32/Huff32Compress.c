#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "Huff32Entry.h"
#include "Node32.h"
#include "../BufView32.h"

#include "Huff32Compress.h"

#define OUT_TEXT_MAX_SIZE 100000
#define MAX_CHARS 100000

// Expect that we don't get too many unique symbols
#define MAX_UNIQUE_SYMBOLS 65536

// Returns the complete table, giving count for each char
Huff32Entry *getUniqueSymbols32(const char *text, int *numSymbols) {
	// Used to count each char
	Huff32Entry staticTable[MAX_UNIQUE_SYMBOLS];

	// Start all counts at 0
	memset(staticTable, 0, MAX_UNIQUE_SYMBOLS*sizeof(Huff32Entry));

	int uniques = 0;

	for (int i = 0; text[i] != 0; i += 4) {
		const unsigned int symbol = readIntFromBuff(i, (unsigned char*)text);

		// Search if we already have this symbol
		bool found = false;
		int j;
		for (j = 0; j < uniques; ++j) {
			if (staticTable[j].symbol == symbol) {
				found = true;
				break;
			}
		}

		if (found) {
			staticTable[j].count++;
		} else {
			if (uniques == MAX_UNIQUE_SYMBOLS) {
				printf("Too many unique symbols\n");
				return NULL;
			}

			staticTable[uniques] = (Huff32Entry){symbol, 1};
			++uniques;
		}
	}

	*numSymbols = uniques;
	printf("Found %i unique symbols\n", uniques);

	// Create entry table
	Huff32Entry *entries = malloc(uniques*sizeof(Huff32Entry));
	if (entries == NULL) {
		printf("Couldn't allocate entry table");
		return NULL;
	}

	// Copy data from static table to heap memory
	// memcpy_s(entries, uniques*sizeof(Huff32Entry), staticTable, MAX_UNIQUE_SYMBOLS*sizeof(Huff32Entry));
	memcpy(entries, staticTable, uniques*sizeof(Huff32Entry));

	return entries;
}

void sortEntries32(Huff32Entry *entries, const int numSymbols) {
	for (int i = 0; i < numSymbols-1; ++i) {
		for (int j = 1; j < numSymbols-i; ++j) {
			if (entries[j-1].count < entries[j].count) {
				const Huff32Entry temp = entries[j-1];
				entries[j-1] = entries[j];
				entries[j] = temp;
			}
		}
	}
}

errno_t createHuffman32Tree(Node32 nodes[MAX_NODES], const Huff32Entry *entries, const int numSymbols, int *outNumNodes) {
	int numNodes = 0;

	for (int i = 0; i < numSymbols; ++i) {
		nodes[i] = (Node32){entries[i].symbol, entries[i].count, NULL, NULL, NULL, false};
	}

	numNodes = numSymbols;

	int i = numNodes-1;

	// While there are at least 2 root nodes
	printf("Creating huffman tree\n");
	while (i >= 1) {
		Node32 nodeA = nodes[i-1];
		Node32 nodeB = nodes[i];

		// Create a new parent node
		Node32 parent = (Node32){0, nodeA.count + nodeB.count, NULL, NULL, NULL, false};

		// Allocate space for each child node
		parent.left = malloc(sizeof(Node32));
		if (parent.left == NULL) {
			printf("Couldn't allocate memory\n");
			return 1;
		}

		parent.right = malloc(sizeof(Node32));
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

CompStream createCompressedText32(const char *text, Node32 *root) {
	printf("Allocating stream\n");
	CompStream out = EMPTY_COMP_STREAM;
	out.text = calloc(OUT_TEXT_MAX_SIZE, sizeof(char));
	if (out.text == NULL) {
		printf("Couldn't allocate memory\n");
		return out;
	}

	printf("Stream allocated\n");

	Node32 nodePath[MAX_NODE_DEPTH];
	int pathLen = 0;

	int i = 0;

	printf("Getting symbol\n");
	unsigned int symbol = readIntFromBuff(0, (unsigned char*)text);

	printf("Starting decompression\n");
	while (symbol != 0) {
		bool found = findPathForSymbol32(nodePath, root, &pathLen, symbol);

		if (!found) {
			printf("Couldn't find symbol '%c%c%c%c' ('%i')\n", text[i], text[i+1], text[i+2], text[i+3], symbol);
			free(out.text);
			return EMPTY_COMP_STREAM;
		}

		for (int j = 1; j < pathLen; ++j) {
			unsigned char byte = 0;
			Node32 curNode = nodePath[j];

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

		i += 4;
		symbol = readIntFromBuff(i, (unsigned char*)text);
	}

	// Calculate the length of the output string
	printf("Calculating length of compressed text\n");
	out.length = out.nextByteIndex + 1 - (out.nextBitIndex==0);

	return out;
}

WriteNode32 *createWriteTable32(Node32 *root, const int numNodes) {
	WriteNode32 *nodeList = malloc(numNodes*sizeof(WriteNode32));
	int curNodeIndex = 0;

	placeNode32InList(root, nodeList, numNodes, &curNodeIndex, -1);

	return nodeList;
}

errno_t writeAllDataToBuffer32(WriteNode32 *nodeList, const int numNodes, CompStream stream, char **outOut, int *outLen) {
	// Write to a file
	const int bytesNeeded =
		4 + // Number of nodes
		CHARS_PER_WRITE_NODE_32 * numNodes + // Table
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
		const int buffIndex = 12 + i * CHARS_PER_WRITE_NODE_32;

		WriteNode32 node = nodeList[i];
		
		// 0 -> 3
		writeIntToBuff(node.parent, buffIndex, (unsigned char*)out);

		// 4 -> 7
		writeIntToBuff(node.symbol, buffIndex+4, (unsigned char*)out);

		// 8
		out[buffIndex+8] = (char)node.isRight;
	}

	// for (int i = 0; i < numNodes; ++i) {
	// 	const int buffIndex = 12 + i * CHARS_PER_WRITE_NODE_32;
	// 	printf("Node\n");
	// 	printf("%i\n", readIntFromBuff(buffIndex, out));
	// 	printf("%c%c\n", out[buffIndex+4], out[buffIndex+5]);
	// 	printf("%i\n", out[buffIndex+6]);
	// 	printf("\n");
	// }

	// Write compressed text
	printf("Writing compressed text\n");
	const int textStartIndex = 12 + CHARS_PER_WRITE_NODE_32 * numNodes;
	for (int i = 0; i <= lastByteIndex; ++i) {
		out[textStartIndex+i] = stream.text[i];
	}

	*outOut = out;
	*outLen = bytesNeeded;

	return 0;
}

// Compresses using huffman and writes directly to the file
errno_t huff32CompressFile(const char *infilename, const char *outfilename) {
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

	return huff32Compress(text, outfilename);
}

// Compresses using huffman and writes directly to the file
errno_t huff32Compress(const char *text, const char *outfilename) {
	// Count each character
	int numSymbols;
	Huff32Entry *entries = getUniqueSymbols32(text, &numSymbols);
	if (entries == NULL) {
		printf("Couldn't get unique symbols\n");
		return 1;
	}

	// Sort entries
	sortEntries32(entries, numSymbols);

	Node32 nodes[MAX_NODES];
	int numNodes = 0;

	errno_t err = createHuffman32Tree(nodes, entries, numSymbols, &numNodes);
	if (err) {
		printf("Couldn't create huffman tree\n");
		return 1;
	}

	// Destroy symbol counts
	printf("Destroying symbol counts\n");
	free(entries);

	// Keep track of the root node
	printf("Picking root node\n");
	Node32 root = nodes[0];

	// Get the number of nodes
	numNodes = countNode32s(&root);
	printf("Tree made of %i nodes\n", numNodes);

	// Fix parents
	printf("Fixing parents\n");
	fixParents32(&root);

	// Use the tree to compress the data
	printf("Creating comp stream\n");
	CompStream stream = createCompressedText32(text, &root);
	if (stream.text == NULL) {
		printf("Couldn't compress text\n");
		return 1;
	}

	printf("Compressed Text Length: %i\n", stream.length);

	// Compress tree to a writable table
	WriteNode32 *nodeList = createWriteTable32(&root, numNodes);

	// Destroy the huffman tree
	printf("Destroying huffman tree\n");
	destroyNode32(&root);

	char *out;
	int outLen;
	err = writeAllDataToBuffer32(nodeList, numNodes, stream, &out, &outLen);
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
