#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "Huff16Entry.h"
#include "Node16.h"
#include "../BufView32.h"

#include "Huff16Compress.h"

#define OUT_TEXT_MAX_SIZE 100000
#define MAX_CHARS 100000
#define MAX_UNIQUE_SYMBOLS 65536

// Returns the complete table, giving count for each char
Huff16Entry *getUniqueSymbols16(const char *text, int *numSymbols) {
	// Used to count each char
	int staticTable[MAX_UNIQUE_SYMBOLS]; // NOTE: Good, but unreasonable for int32

	// Start all counts at 0
	memset(staticTable, 0, MAX_UNIQUE_SYMBOLS*sizeof(int));

	// Count each symbol
	for (int i = 0; text[i] != 0; i += 2) {
		const unsigned short symbol = ((short)(text[i])<<8) | (short)text[i+1];
		staticTable[symbol]++;
	}

	// Count how many uniques there are
	int uniques = 0;
	for (int i = 0; i < MAX_UNIQUE_SYMBOLS; ++i) {
		if (staticTable[i] != 0) {
			++uniques;
		}
	}
	*numSymbols = uniques;
	printf("Found %i unique symbols\n", uniques);

	// Create entry table
	Huff16Entry *entries = malloc(uniques*sizeof(Huff16Entry));
	if (entries == NULL) {
		printf("Couldn't allocate entry table");
		return NULL;
	}

	// Place counts into table
	int tableIndex = 0;
	for (int i = 0; i < MAX_UNIQUE_SYMBOLS; ++i) {
		if (staticTable[i] == 0) {
			continue;
		}

		entries[tableIndex].symbol = (short)i;
		entries[tableIndex].count = staticTable[i];
		++tableIndex;
	}

	return entries;
}

void sortEntries16(Huff16Entry *entries, const int numSymbols) {
	for (int i = 0; i < numSymbols-1; ++i) {
		for (int j = 1; j < numSymbols-i; ++j) {
			if (entries[j-1].count < entries[j].count) {
				const Huff16Entry temp = entries[j-1];
				entries[j-1] = entries[j];
				entries[j] = temp;
			}
		}
	}
}

errno_t createHuffman16Tree(Node16 nodes[MAX_NODES], const Huff16Entry *entries, const int numSymbols, int *outNumNodes) {
	int numNodes = 0;

	for (int i = 0; i < numSymbols; ++i) {
		nodes[i] = (Node16){entries[i].symbol, entries[i].count, NULL, NULL, NULL, false};
	}

	numNodes = numSymbols;

	int i = numNodes-1;

	// While there are at least 2 root nodes
	printf("Creating huffman tree\n");
	while (i >= 1) {
		Node16 nodeA = nodes[i-1];
		Node16 nodeB = nodes[i];

		// Create a new parent node
		Node16 parent = (Node16){0, nodeA.count + nodeB.count, NULL, NULL, NULL, false};

		// Allocate space for each child node
		parent.left = malloc(sizeof(Node16));
		if (parent.left == NULL) {
			printf("Couldn't allocate memory\n");
			return 1;
		}

		parent.right = malloc(sizeof(Node16));
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

CompStream createCompressedText16(const char *text, Node16 *root) {
	printf("Allocating stream\n");
	CompStream out = EMPTY_COMP_STREAM;
	out.text = calloc(OUT_TEXT_MAX_SIZE, sizeof(char));
	if (out.text == NULL) {
		printf("Couldn't allocate memory\n");
		return out;
	}

	printf("Stream allocated\n");

	Node16 nodePath[MAX_NODE_DEPTH];
	int pathLen = 0;

	int i = 0;

	printf("Getting symbol\n");
	short symbol = (((short)(text[0]))<<8) | ((short)(text[1]));

	printf("Starting decompression\n");
	while (symbol != 0) {
		bool found = findPathForSymbol16(nodePath, root, &pathLen, symbol);

		if (!found) {
			printf("Couldn't find symbol '%c%c' ('%i')\n", text[i], text[i+1], symbol);
			free(out.text);
			return EMPTY_COMP_STREAM;
		}

		for (int j = 1; j < pathLen; ++j) {
			unsigned char byte = 0;
			Node16 curNode = nodePath[j];

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

		i += 2;
		symbol = ((short)(text[i])<<8) | (short)(text[i+1]);
	}

	// Calculate the length of the output string
	printf("Calculating length of compressed text\n");
	out.length = out.nextByteIndex + 1 - (out.nextBitIndex==0);

	return out;
}

WriteNode16 *createWriteTable16(Node16 *root, const int numNodes) {
	WriteNode16 *nodeList = malloc(numNodes*sizeof(WriteNode16));
	int curNodeIndex = 0;

	placeNode16InList(root, nodeList, numNodes, &curNodeIndex, -1);

	return nodeList;
}

errno_t writeAllDataToBuffer16(WriteNode16 *nodeList, const int numNodes, CompStream stream, char **outOut, int *outLen) {
	// Write to a file
	const int bytesNeeded =
		4 + // Number of nodes
		CHARS_PER_WRITE_NODE_16 * numNodes + // Table
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
		const int buffIndex = 12 + i * CHARS_PER_WRITE_NODE_16;

		WriteNode16 node = nodeList[i];
		
		// 0 -> 3
		writeIntToBuff(node.parent, buffIndex, (unsigned char*)out);

		// 4 -> 5
		out[buffIndex+4] = (char)(node.symbol>>8);
		out[buffIndex+5] = (char)(node.symbol);

		// 6
		out[buffIndex+6] = (char)node.isRight;
	}

	// for (int i = 0; i < numNodes; ++i) {
	// 	const int buffIndex = 12 + i * CHARS_PER_WRITE_NODE_16;
	// 	printf("Node\n");
	// 	printf("%i\n", readIntFromBuff(buffIndex, out));
	// 	printf("%c%c\n", out[buffIndex+4], out[buffIndex+5]);
	// 	printf("%i\n", out[buffIndex+6]);
	// 	printf("\n");
	// }

	// Write compressed text
	printf("Writing compressed text\n");
	const int textStartIndex = 12 + CHARS_PER_WRITE_NODE_16 * numNodes;
	for (int i = 0; i <= lastByteIndex; ++i) {
		out[textStartIndex+i] = stream.text[i];
	}

	*outOut = out;
	*outLen = bytesNeeded;

	return 0;
}

// Compresses using huffman and writes directly to the file
errno_t huff16CompressFile(const char *infilename, const char *outfilename) {
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

	return huff16Compress(text, outfilename);
}

// Compresses using huffman and writes directly to the file
errno_t huff16Compress(const char *text, const char *outfilename) {
	// Count each character
	int numSymbols;
	Huff16Entry *entries = getUniqueSymbols16(text, &numSymbols);
	if (entries == NULL) {
		printf("Couldn't get unique symbols\n");
		return 1;
	}

	// Sort entries
	sortEntries16(entries, numSymbols);

	Node16 nodes[MAX_NODES];
	int numNodes = 0;

	errno_t err = createHuffman16Tree(nodes, entries, numSymbols, &numNodes);
	if (err) {
		printf("Couldn't create huffman tree\n");
		return 1;
	}

	// Destroy symbol counts
	printf("Destroying symbol counts\n");
	free(entries);

	// Keep track of the root node
	printf("Picking root node\n");
	Node16 root = nodes[0];

	// Get the number of nodes
	numNodes = countNode16s(&root);
	printf("Tree made of %i nodes\n", numNodes);

	// Fix parents
	printf("Fixing parents\n");
	fixParents16(&root);

	// Use the tree to compress the data
	printf("Creating comp stream\n");
	CompStream stream = createCompressedText16(text, &root);
	if (stream.text == NULL) {
		printf("Couldn't compress text\n");
		return 1;
	}

	printf("Compressed Text Length: %i\n", stream.length);

	// Compress tree to a writable table
	WriteNode16 *nodeList = createWriteTable16(&root, numNodes);

	// Destroy the huffman tree
	printf("Destroying huffman tree\n");
	destroyNode16(&root);

	char *out;
	int outLen;
	err = writeAllDataToBuffer16(nodeList, numNodes, stream, &out, &outLen);
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
