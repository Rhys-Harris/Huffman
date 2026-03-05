#include <corecrt.h>
#include <stdio.h>

#include "Huff8/Huff.h"
#include "Huff16/Huff16.h"

errno_t testHuff8(const char infilename[]) {
	printf("== TESTING HUFF 8 ==\n\n");

	if (huffCompressFile(infilename, "out.huff")) {
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

	printf("== END TESTING HUFF 8 ==\n\n");

	return 0;
}

errno_t testHuff16(const char infilename[]) {
	printf("== TESTING HUFF 16 ==\n\n");

	if (huff16CompressFile(infilename, "out.huff16")) {
		printf("Couldn't compress\n");
		return 1;
	}

	printf("FINISHED COMPRESSION\n");
	printf("\n\n");

	if (huff16DecompressFile("out.huff16", "out16.txt")) {
		printf("Couldn't decompress\n");
		return 1;
	}
	
	printf("FINISHED DECOMPRESSION\n");

	printf("== END TESTING HUFF 16 ==\n\n");

	return 0;
}

int main(const int argc, char *argv[]) {
	const char infilename[] = "out.ppm";

	if (testHuff8(infilename)) {
		printf("Fail on testing huff8\n");
		return 1;
	}

	if (testHuff16(infilename)) {
		printf("Fail on testing huff16\n");
		return 1;
	}

	return 0;
}
