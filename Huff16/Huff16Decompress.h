#ifndef _HUFF_16_DECOMPRESS_H_
#define _HUFF_16_DECOMPRESS_H_

#include <corecrt.h>

errno_t huff16Decompress(const char *compText, const char *outfilename);

errno_t huff16DecompressFile(const char *infilename, const char *outfilename);

#endif
