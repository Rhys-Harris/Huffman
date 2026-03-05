#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Huff32Decompress.h"
#include "../BufView32.h"
#include "ReadNode32.h"
#include "WriteNode32.h"

#define MAX_OUTPUT_CHARS 200000

#define MAX_CHARS 100000

int decompress32(const char *inText, char *out, int outLength, ReadNode32 *table, const int lastByteIndex, const int lastBitIndex) {
	// Each bit
	//	Traverse tree
	int curByte = 0;
	int curBit = 0;

	ReadNode32 curNode = table[0];

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
			// printf("'%c%c' (%i)\n", curNode.symbol>>8, curNode.symbol, curNode.symbol);
			writeIntToBuff(curNode.symbol, outLen, (unsigned char*)out);
			curNode = table[0];
			outLen += 4;
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
errno_t huff32DecompressFile(const char *infilename, const char *outfilename) {
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

	return huff32Decompress(text, outfilename);
}

// Compresses using huffman and writes directly to the file
errno_t huff32Decompress(const char *compText, const char *outfilename) {
	// Get metadata
	printf("Getting metadata\n");
	const int numNodes = readIntFromBuff(0, (unsigned char*)compText);
	const int lastByteIndex = readIntFromBuff(4, (unsigned char*)compText);
	const int lastBitIndex = readIntFromBuff(8, (unsigned char*)compText);

	printf("Num nodes %i\n", numNodes);
	printf("Last byte index %i\n", lastByteIndex);
	printf("Last bit index %i\n", lastBitIndex);
	printf("\n");
	
	// Convert raw data to read nodes table
	printf("Allocating table\n");
	WriteNode32 *rawTable = malloc(numNodes*sizeof(WriteNode32));
	if (rawTable == NULL) {
		printf("Couldn't allocate memory\n");
		return 1;
	}
	memset(rawTable, 0, numNodes*sizeof(WriteNode32));

	printf("Reading in table\n");
	for (int i = 0; i < numNodes; ++i) {
		const int buffIndex = 12 + i * CHARS_PER_WRITE_NODE_32;

		WriteNode32 node;
		
		// 0 -> 3
		node.parent = readIntFromBuff(buffIndex, (unsigned char*)compText);

		// 4 -> 7
		node.symbol = readIntFromBuff(buffIndex+4, (unsigned char*)compText);

		// 8
		node.isRight = compText[buffIndex+8];

		rawTable[i] = node;
	}

	printf("\n\n");

	// Convert to readable nodes
	printf("Converting to readable table\n");
	ReadNode32 *table = malloc(numNodes*sizeof(ReadNode32));
	if (table == NULL) {
		printf("Couldn't allocate table\n");
		return 1;
	}

	memset(table, 0, numNodes*sizeof(ReadNode32));

	printf("Converting...\n");

	for (int i = 0; i < numNodes; ++i) {
		WriteNode32 wn = rawTable[i];

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
	const int compBuffIndex = 12 + CHARS_PER_WRITE_NODE_32 * numNodes;
	const char *inText = compText + compBuffIndex;

	// Create output buffer
	char out[MAX_OUTPUT_CHARS];
	memset(out, 0, MAX_OUTPUT_CHARS);

	printf("Decompressing...\n");
	int outLen = decompress32(inText, out, MAX_OUTPUT_CHARS, table, lastByteIndex, lastBitIndex);

	printf("Output String Length %i\n", outLen);
	// printf("%s\n", out);

	// Write output to file
	FILE *f;
	errno_t err = fopen_s(&f, outfilename, "wb");
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
