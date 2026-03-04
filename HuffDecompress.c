#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "HuffDecompress.h"
#include "BufView32.h"
#include "ReadNode.h"
#include "WriteNode.h"

#define MAX_OUTPUT_CHARS 100000

int decompress(const char *inText, char *out, int outLength, ReadNode *table, const int lastByteIndex, const int lastBitIndex) {
	// Each bit
	//	Traverse tree
	int curByte = 0;
	int curBit = 0;

	ReadNode curNode = table[0];

	int outLen = 0;

	while (curByte < lastByteIndex || (curByte == lastByteIndex && curBit <= lastBitIndex)) {
		const char bit = (char)(inText[curByte]>>(7-curBit))&1;

		if (bit == 1) {
			curNode = table[curNode.right];
		} else {
			curNode = table[curNode.left];
		}

		// Hit a leaf!
		if (curNode.left == 0 && curNode.right == 0) {
			out[outLen] = curNode.symbol;
			curNode = table[0];
			++outLen;
		}

		++curBit;
		if (curBit == 8) {
			curBit = 0;
			++curByte;
		}
	}

	return outLen;
}

// Compresses using huffman and writes directly to the file
errno_t huffDecompress(const char *compText, const char *outfilename) {
	// Get metadata
	printf("Getting metadata\n");
	const int numNodes = readIntFromBuff(0, compText);
	const int lastByteIndex = readIntFromBuff(4, compText);
	const int lastBitIndex = readIntFromBuff(8, compText);

	printf("Num nodes %i\n", numNodes);
	printf("Last byte index %i\n", lastByteIndex);
	printf("Last bit index %i\n", lastBitIndex);
	printf("\n");
	
	// Convert raw data to read nodes table
	printf("Allocating table\n");
	WriteNode *rawTable = malloc(numNodes*sizeof(WriteNode));
	if (rawTable == NULL) {
		printf("Couldn't allocate memory\n");
		return 1;
	}
	memset(rawTable, 0, numNodes*sizeof(WriteNode));

	printf("Reading in table\n");
	for (int i = 0; i < numNodes; ++i) {
		const int buffIndex = 12 + i * CHARS_PER_WRITE_NODE;

		WriteNode node;
		
		// 0 -> 3
		node.parent = readIntFromBuff(buffIndex, compText);

		// 4
		node.symbol = compText[buffIndex+4];

		// 5
		node.isRight = compText[buffIndex+5];

		rawTable[i] = node;
	}

	printf("\n\n");

	// Convert to readable nodes
	printf("Converting to readable table\n");
	ReadNode *table = malloc(numNodes*sizeof(ReadNode));
	if (table == NULL) {
		printf("Couldn't allocate table\n");
		return 1;
	}

	memset(table, 0, numNodes*sizeof(ReadNode));

	printf("Converting...\n");

	for (int i = 0; i < numNodes; ++i) {
		WriteNode wn = rawTable[i];

		table[i].symbol = wn.symbol;
		if (wn.parent == -1) {
			continue;
		}

		if (wn.isRight) {
			table[wn.parent].right = i;
		} else {
			table[wn.parent].left = i;
		}
	}

	// Destroy write nodes
	printf("Destroying raw table\n");
	free(rawTable);
	printf("\n");

	// Isolate compressed text
	printf("Isolating compressed text\n");
	const int compBuffIndex = 12 + CHARS_PER_WRITE_NODE * numNodes;
	const char *inText = compText + compBuffIndex;

	// Create output buffer
	char out[MAX_OUTPUT_CHARS];
	memset(out, 0, MAX_OUTPUT_CHARS);

	printf("Decompressing...\n");
	int outLen = decompress(inText, out, MAX_OUTPUT_CHARS, table, lastByteIndex, lastBitIndex);

	printf("Output String Length %i\n", outLen);

	// Write output to file
	FILE *f;
	errno_t err = fopen_s(&f, outfilename, "w");
	if (err) {
		printf("Couldn't open output file\n");
		return 1;
	}
	fwrite(out, sizeof(char), outLen, f);
	fclose(f);

	// Destroy table
	free(table);

	return 0;
}
