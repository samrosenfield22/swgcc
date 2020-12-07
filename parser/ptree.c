

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

	assert(sym == NULL);	//if this never triggers, get rid of the argument

	n->is_nonterminal = is_nonterminal;
    n->ntype = ntype;
    n->sym = sym;
    n->lval = false;
    n->block_bytes = 0;

    return n;
}

/* walks a path along the parse tree, specified by the given string pathspec
ex. ptree_walk_path(node, "parent.decl");	walks the path node->parent->child nonterm decl

more generally, pathspec is a series of motions, which are delimited by '.', where a motion is
one of the following:

"parent"	gets the parent of the current node
"decl"		gets the first child of the current node which is of matching nonterminal type
"1"			gets child 1 of the current node

if any motion cannot be made (ex. the node doesn't have the specifiec child), ptree_walk_path()
returns NULL			
*/
pnode *ptree_walk_path(pnode *start, const char *pathspec)
{
	pnode *current = start;
	char *paths = strdup(pathspec);

	char *motion = paths, *end;

	while(1)
	{
		end = strchr(motion, '.');
		if(end) *end = '\0';

		if(strcmp(motion, "parent")==0)			current = current->parent;
		else if('a'<=*motion && *motion<='z')	current = get_nonterm_child(current, motion);
		else									current = current->children[atoi(motion)];

		if(!current)	{free(paths); return NULL;}
		
		if(!end) break;
		motion = end+1;
	}

	free(paths);
	return current;
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
		ptree_traverse(pt, pnode_print_str(n), true);

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

///////////////////////////////

//returns the first child that matches
pnode *get_nonterm_child(pnode *parent, char *ntstr)
{

	int index = get_nonterm_child_index(parent, ntstr);
	return (index==-1)? NULL : parent->children[index];
}

int get_nonterm_child_index(pnode *parent, char *ntstr)
{
	vector_foreach(parent->children, i)
	{
		if(is_nonterm_type(parent->children[i], ntstr))
			return i;
	}
	return -1;
}

/*static int get_semact_child_index(pnode *parent, char *str)
{
	vector_foreach(parent->children, i)
	{
		//pnode *c = parent->children[i];
		//if(!c->is_nonterminal && c->ntype==SEMACT && strcmp(c->str, str)==0)
		//	return i;
		if(is_semact_type(parent->children[i], str))
			return i;
	}
	return -1;
}*/

/*static bool is_semact_type(pnode *n, const char *sem)
{
	if(n->is_nonterminal || n->ntype!=SEMACT)
		return false;
	return (strcmp(n->str, sem)==0);
}*/

bool is_semact_special(pnode *n)
{
	if(n->is_nonterminal || n->ntype!=SEMACT)
		return false;
	return (n->str[0] == '!' && n->str[1] == ' ');
}

//looks all the way through the tree, returns the first nonterm match
pnode *get_nonterm_child_deep(pnode *parent, char *ntstr)
{
	pnode **matches = tree_filter(parent, is_nonterm_type(n, ntstr) && n!=parent, true);
	pnode *child = vector_len(matches)? matches[0] : NULL;
	vector_destroy(matches);
	return child;
}

pnode *get_semact_child(pnode *parent, char *ntstr)
{
	vector_foreach(parent->children, i)
	{
		pnode *c = parent->children[i];
		if(!c->is_nonterminal && c->ntype==SEMACT && strcmp(c->str, ntstr)==0)
			return parent->children[i];
	}
	return NULL;
}

bool is_nonterm_type(pnode *n, const char *ntstr)
{
	//for testing only -- if i mistype the nonterm string, this will catch it
	if(vector_search_str(gg.nonterminals, ntstr) == -1)
	{
		printf("typo in nonterm name \"%s\" passed to is_nonterm_type (semantic.c)\n", ntstr);
		assert(0);
	}

	if(!(n->is_nonterminal))
		return false;
	return (strcmp(gg.nonterminals[n->ntype], ntstr)==0);
}

pnode *get_nonterm_ancestor(pnode *n, const char *ntstr)
{
	assert(n);
	while(1)
	{
		n = tree_get_parent(n);
		if(!n) break;
		
		if(is_nonterm_type(n, ntstr))
			return n;
	}
	return NULL;
}
