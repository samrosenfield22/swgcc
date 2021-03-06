

#ifndef PTREE_H_
#define PTREE_H_

#include "../ds/tree.h"

#include "grammar.h"
#include "recdesc_types.h"
#include "../ds/vector.h"
#include "../utils/printcolor.h"

extern const char *t_strings[];

//parse tree node
//extend the tree node type
typedef struct pnode_s pnode;
struct pnode_s
{
	//node n;	//base tree node. must be the first struct element!
	struct
	{
		pnode *parent;
	    pnode **children;
	    char *str;
	};

	//extra elements:
	bool is_nonterminal;
	int ntype;	//either a nonterminal index, or a type of thing (nonterminal, semantic, ident...)

    //symbol *type;

    symbol *sym;	//if the node contains a variable, this points to its symbol in the sym table

    bool lval;
    size_t block_bytes;	//only for "block" nonterms -- number of auto/local variable bytes
};

//extern node ref_node;	//defined in ptree.c

//
pnode *pnode_create(bool is_nonterminal, int ntype, const char *str, symbol *sym);
pnode *ptree_walk_path(pnode *start, const char *pathspec);
//bool filter_by_ref_node(node *n);
void ptree_print(void *pt);
void pnode_print(void *pt);
void pnode_print_str(void *pt);
void semact_print(void *pt);

//////

pnode *get_nonterm_child(pnode *parent, char *ntstr);
int get_nonterm_child_index(pnode *parent, char *ntstr);
pnode *get_semact_child(pnode *parent, char *ntstr);
//int get_semact_child_index(pnode *parent, char *str);
//bool is_semact_type(pnode *n, const char *sem);
bool is_semact_special(pnode *n);
pnode *get_nonterm_child_deep(pnode *parent, char *ntstr);
bool is_nonterm_type(pnode *n, const char *ntstr);
pnode *get_nonterm_ancestor(pnode *n, const char *ntstr);

#define ptree_traverse 		tree_traverse
#define ptree_filter 		tree_filter

#endif //PTREE_H_