#ifndef _WRITE_NODE_16_H_
#define _WRITE_NODE_16_H_

#include <stdbool.h>

#define CHARS_PER_WRITE_NODE_16 (2+4+1)

typedef struct WriteNode16 {
		short symbol;
		int parent;
		bool isRight;
} WriteNode16;

#endif
