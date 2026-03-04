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

	// Read in text just compressed
	FILE *f;
	if (fopen_s(&f, "out.huff", "rb")) {
		printf("Couldn't open output file\n");
		return 1;
	}
	char compText[MAX_CHARS];
	printf("Read %lli chars\n", fread(compText, sizeof(char), MAX_CHARS, f));
	fclose(f);

	// printf("%s\n", compText);

	if (huffDecompress(compText, "out.txt")) {
		printf("Couldn't decompress\n");
		return 1;
	}
	
	printf("FINISHED DECOMPRESSION\n");

	return 0;
}
