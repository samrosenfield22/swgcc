

#ifndef TYPESYS_H_
#define TYPESYS_H_

#include "ptree.h"
#include "../ds/vector.h"
#include "../utils/misc.h"
//#include "../ds/tree.h"
//#include "../symbol.h"


typedef struct typenode_s typenode;

//void init_type_system(void);
void resolve_type(symbol *sym, pnode *dcltor, char *type);
void print_type(typenode *t);

#endif //TYPESYS_H_