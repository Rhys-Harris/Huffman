#ifndef _WRITE_NODE_H_
#define _WRITE_NODE_H_

#include <stdbool.h>

#define CHARS_PER_WRITE_NODE (1+1+4)

typedef struct WriteNode {
		char symbol;
		int parent;
		bool isRight;
} WriteNode;

#endif
