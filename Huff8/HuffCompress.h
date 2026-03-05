#ifndef _HUFF_COMPRESS_H_
#define _HUFF_COMPRESS_H_

#include <corecrt.h>

// Compresses using huffman and writes directly to the file
errno_t huffCompress(const char *text, const char *outfilename);

errno_t huffCompressFile(const char *infilename, const char *outfilename);

#endif
