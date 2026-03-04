#include <stdio.h>

#include "Huff.h"

#define MAX_CHARS 100000

int main(const int argc, char *argv[]) {
	// for (int i = 0; i < argc; ++i) {
	// 	printf("ARG %i: %s\n", i, argv[i]);
	// }

	if (huffCompressFile("out.ppm", "out.huff")) {
		printf("Couldn't compress\n");
		return 1;
	}

	printf("FINISHED COMPRESSION\n");
	printf("\n\n");

	if (huffDecompressFile("out.huff", "out.txt")) {
		printf("Couldn't decompress\n");
		return 1;
	}
	
	printf("FINISHED DECOMPRESSION\n");

	return 0;
}
