#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Huff16Decompress.h"
#include "../BufView32.h"
#include "ReadNode16.h"
#include "WriteNode16.h"

#define MAX_OUTPUT_CHARS 200000

#define MAX_CHARS 100000

int decompress16(const char *inText, char *out, int outLength, ReadNode16 *table, const int lastByteIndex, const int lastBitIndex) {
	// Each bit
	//	Traverse tree
	int curByte = 0;
	int curBit = 0;

	ReadNode16 curNode = table[0];

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
			out[outLen] = (char)(curNode.symbol>>8);
			out[outLen+1] = (char)(curNode.symbol);
			curNode = table[0];
			outLen += 2;
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
errno_t huff16DecompressFile(const char *infilename, const char *outfilename) {
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

	return huff16Decompress(text, outfilename);
}

// Compresses using huffman and writes directly to the file
errno_t huff16Decompress(const char *compText, const char *outfilename) {
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
	WriteNode16 *rawTable = malloc(numNodes*sizeof(WriteNode16));
	if (rawTable == NULL) {
		printf("Couldn't allocate memory\n");
		return 1;
	}
	memset(rawTable, 0, numNodes*sizeof(WriteNode16));

	printf("Reading in table\n");
	for (int i = 0; i < numNodes; ++i) {
		const int buffIndex = 12 + i * CHARS_PER_WRITE_NODE_16;

		WriteNode16 node;
		
		// 0 -> 3
		node.parent = readIntFromBuff(buffIndex, compText);

		// 4 -> 5
		node.symbol = ((short)(compText[buffIndex+4])<<8) | ((short)(compText[buffIndex+5]));

		// 6
		node.isRight = compText[buffIndex+6];

		rawTable[i] = node;
	}

	printf("\n\n");

	// Convert to readable nodes
	printf("Converting to readable table\n");
	ReadNode16 *table = malloc(numNodes*sizeof(ReadNode16));
	if (table == NULL) {
		printf("Couldn't allocate table\n");
		return 1;
	}

	memset(table, 0, numNodes*sizeof(ReadNode16));

	printf("Converting...\n");

	for (int i = 0; i < numNodes; ++i) {
		WriteNode16 wn = rawTable[i];

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
	const int compBuffIndex = 12 + CHARS_PER_WRITE_NODE_16 * numNodes;
	const char *inText = compText + compBuffIndex;

	// Create output buffer
	char out[MAX_OUTPUT_CHARS];
	memset(out, 0, MAX_OUTPUT_CHARS);

	printf("Decompressing...\n");
	int outLen = decompress16(inText, out, MAX_OUTPUT_CHARS, table, lastByteIndex, lastBitIndex);

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
