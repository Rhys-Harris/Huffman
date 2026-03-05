#ifndef _HUFF_16_COMPRESS_H_
#define _HUFF_16_COMPRESS_H_

#include <corecrt.h>

// Compresses using huffman and writes directly to the file
errno_t huff16Compress(const char *text, const char *outfilename);

errno_t huff16CompressFile(const char *infilename, const char *outfilename);

#endif
