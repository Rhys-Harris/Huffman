#ifndef _WRITE_NODE_32_H_
#define _WRITE_NODE_32_H_

#include <stdbool.h>

#define CHARS_PER_WRITE_NODE_32 (4+4+1)

typedef struct WriteNode32 {
	unsigned int symbol;
	int parent;
	bool isRight;
} WriteNode32;

#endif
