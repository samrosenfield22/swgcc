

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "tree.h"

/*const char *nt_strings[] =
{
	[REGEX] = "regex",
	[MORETERM] = "moreterm",
	[TERM] = "term",
	[FACTOR] = "factor",
	[CONFACTOR] = "confactor",
	[BASE_SUFFIX] = "base_suffix",
	[BASE] = "base",
	[RANGE] = "range",
};*/
char **nt_strings;

const char *t_strings[] =
{
	[0] = "SHOULDNTSEETHIS",
	[IDENT] = "ident",
	[TERMINAL] = "terminal",
	[SEMACT] = "semact",
	[EXPR] = "expr"
};


node *node_create(bool is_nonterminal, int type, const char *str)
{
    node *n = malloc(sizeof(*n));
    assert(n);

	n->is_nonterminal = is_nonterminal;
    n->type = type;

    for(int i=0; i<NODE_CHILDREN_CT; i++)
        n->children[i] = NULL;
    n->next_child = 0;

    if(str)
    {
    	n->str = malloc(strlen(str)+1);
    	assert(n->str);
    	strcpy(n->str, str);
    }
    else
    	n->str = NULL;

    return n;
}

void node_add_child(node *root, node *child)
{
	root->children[root->next_child] = child;
	root->next_child++;
}

void ptree_init_names(char **strings)
{
	nt_strings = strings;
}

void ptree_traverse_dfs(node *pt, void (*action)(node *pt, int arg), bool node_then_children)
{
		ptree_traverse_dfs_recursive(pt, action, 0, node_then_children);
}

void ptree_traverse_dfs_recursive(node *pt, void (*action)(node *pt, int depth), int depth, bool node_then_children)
{
		if(node_then_children)
			action(pt, depth);

		for(int ci=0; pt->children[ci]; ci++)
		{
			ptree_traverse_dfs_recursive(pt->children[ci], action, depth+1, node_then_children);
		}

		if(!node_then_children)
			action(pt, depth);
}

void node_print(node *pt, int depth)
{
	for(int i=0; i<depth; i++)
	{
		printf("  ");
	}
		if(pt->is_nonterminal == true)
			printf("(%s) ", nt_strings[pt->type]);
			//printf("nootnoot ");
		else
			printf("(%s) ", t_strings[pt->type]);

		if(pt->str)
			printf("%s", pt->str);
		putchar('\n');
}

void semact_print(node *pt, int depth)
{
	if(!(pt->is_nonterminal) && pt->type == SEMACT)
		puts(pt->str);
}

void node_delete(node *pt, int dummy)
{
	free(pt);
}