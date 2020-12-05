

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "ptree.h"

//labels for tree printing. if we're printing a nonterminal, we print the nonterm type instead of "nonterminal"
const char *t_strings[] =
{
	[0] = "SHOULDNTSEETHIS",
	[IDENT] = "ident",
	[TERMINAL] = "terminal",
	[SEMACT] = "semact",
	[EXPR] = "expr"
};


pnode *pnode_create(bool is_nonterminal, int ntype, const char *str, symbol *sym)
{
	pnode *n = tree_create_extra(sizeof(*n), str);
	assert(n);

	n->is_nonterminal = is_nonterminal;
    n->ntype = ntype;
    n->sym = sym;
    n->lval = false;
    n->block_bytes = 0;

    return n;
}

//delet, maybe. ptree_filter() macro kinda replaced this functionality.
/*node ref_node;	//declared extern in tree.h
bool filter_by_ref_node(node *n)
{
	assert(0);	//are we nuking this function or what??

	if(n==NULL)
		return false;

	if(n->is_nonterminal != ref_node.is_nonterminal)
		return false;

	if(n->is_nonterminal)
	{
		if(strcmp(gg.nonterminals[n->ntype], ref_node.str))
			return false;
	}
	else
	{
		if(n->ntype != ref_node.ntype)
			return false;
		if(n->str == NULL || strcmp(n->str, ref_node.str))
			return false;
	}

	return true;
}*/

void ptree_print(void *pt)
{
	tree_print_pretty(pt, pnode_print);
}

void pnode_print(void *t)
{
	pnode *pt = (pnode*)t;
	
	bool is_semact = (!pt->is_nonterminal && pt->ntype == SEMACT);

	//print node type
	if(pt->is_nonterminal == true)
		printf("(%s) ", gg.nonterminals[pt->ntype]);
	else
		printf("(%s) ", t_strings[pt->ntype]);

	//print node data
	if(is_semact)
		printf("%s", pt->str);
	else
		ptree_traverse(pt, pnode_print_str(n), false);

	putchar('\n');
}

//build and print the string by recursively adding substrings from child nodes
void pnode_print_str(void *pt)
{
	pnode *p = (pnode *)pt;
	if(!(p->is_nonterminal) && p->ntype != SEMACT)
		if(p->str)
			printf("%s ", p->str);
}

void semact_print(void *pt)
{
	pnode *p = (pnode *)pt;
	if(!(p->is_nonterminal) && p->ntype == SEMACT)
		puts(p->str);
}