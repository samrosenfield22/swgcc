

#ifndef TYPESYS_H_
#define TYPESYS_H_

#include "ptree.h"
#include "../ds/vector.h"
//#include "../ds/tree.h"
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

void init_type_system(void);
void resolve_type(symbol *sym, pnode *dcltor, char *type);

#endif //TYPESYS_H_