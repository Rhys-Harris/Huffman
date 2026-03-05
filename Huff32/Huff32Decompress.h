#ifndef _HUFF_32_DECOMPRESS_H_
#define _HUFF_32_DECOMPRESS_H_

#include <corecrt.h>

errno_t huff32Decompress(const char *compText, const char *outfilename);

errno_t huff32DecompressFile(const char *infilename, const char *outfilename);

#endif
