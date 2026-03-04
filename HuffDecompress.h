#ifndef _HUFF_DECOMPRESS_H_
#define _HUFF_DECOMPRESS_H_

#include <corecrt.h>

errno_t huffDecompress(const char *compText, const char *outfilename);

#endif
