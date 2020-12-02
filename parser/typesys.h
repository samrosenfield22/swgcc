

#ifndef TYPESYS_H_
#define TYPESYS_H_

#include "../ds/tree.h"
//#include "../symbol.h"

typedef struct typetree_s
{
	struct 	//must be first!
	{
		pnode *parent;
	    pnode **children;
	    char *str;
	};

	

} typetree;

#endif //TYPESYS_H_