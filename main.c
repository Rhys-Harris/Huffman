#include <corecrt.h>
#include <stdio.h>

#include "Huff8/Huff.h"
#include "Huff16/Huff16.h"
#include "Huff32/Huff32.h"

typedef errno_t (*compFunc)(const char[], const char[]);

#define NUM_COMP_METHODS 3

char compGroupNum[NUM_COMP_METHODS][2] = {
	"08",
	"16",
	"32",
};

compFunc compMethods[NUM_COMP_METHODS]  = {
	huffCompressFile,
	huff16CompressFile,
	huff32CompressFile,
};

compFunc decompMethods[NUM_COMP_METHODS]  = {
	huffDecompressFile,
	huff16DecompressFile,
	huff32DecompressFile,
};

// We replace the ?? with the correct num
char outCompFileName[] = "compoutputs/out.huff??";
char outDecompFileName[] = "finaloutputs/out??.txt";

#define COMP_OUT_MOD_INDEX 20
#define DECOMP_OUT_MOD_INDEX 16

errno_t testHuffMethod(const char infilename[], const int methodNum) {
	// Fix output file names
	const char *name = compGroupNum[methodNum];
	outCompFileName[COMP_OUT_MOD_INDEX] = name[0];
	outCompFileName[COMP_OUT_MOD_INDEX+1] = name[1];
	outDecompFileName[DECOMP_OUT_MOD_INDEX] = name[0];
	outDecompFileName[DECOMP_OUT_MOD_INDEX+1] = name[1];

	printf("== TESTING HUFF %s ==\n\n", name);

	if (compMethods[methodNum](infilename, outCompFileName)) {
		printf("Couldn't compress\n");
		return 1;
	}

	printf("FINISHED COMPRESSION\n");
	printf("\n\n");

	if (decompMethods[methodNum](outCompFileName, outDecompFileName)) {
		printf("Couldn't decompress\n");
		return 1;
	}
	
	printf("FINISHED DECOMPRESSION\n");

	printf("== END TESTING HUFF %s ==\n\n", name);

	return 0;
}

int main(const int argc, char *argv[]) {
	const char infilename[] = "inputs/out.ppm";

	for (int i = 0; i < NUM_COMP_METHODS; ++i) {
		if (testHuffMethod(infilename, i)) {
			printf("Fail on testing huff%s\n", compGroupNum[i]);
			return 1;
		}
	}

	return 0;
}
