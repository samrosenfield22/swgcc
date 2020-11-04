

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "tree.h"

char **nt_strings;

const char *t_strings[] =
{
	[0] = "SHOULDNTSEETHIS",
	[IDENT] = "ident",
	[TERMINAL] = "terminal",
	[SEMACT] = "semact",
	[EXPR] = "expr"
};


node *node_create(bool is_nonterminal, int type, const char *str, symbol *sym)
{
    node *n = malloc(sizeof(*n));
    assert(n);

	n->is_nonterminal = is_nonterminal;
    n->type = type;
    n->sym = sym;

    //for(int i=0; i<NODE_CHILDREN_CT; i++)
    //    n->children[i] = NULL;
    n->children = vector(node *, 0);
    n->children[0] = NULL;

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
	//root->children[root->next_child] = child;
	//root->next_child++;

	/*node **c;
	for(c = root->children; *c; c++);
	*c = child;*/
	//vector_last(root->children) = child;
	vector_inc(&(root->children));
	vector_last(root->children) = child;
}

/*void ptree_init_names(char **strings)
{
	nt_strings = strings;
}*/

void ptree_traverse_dfs(node *pt, bool (*filter)(node *n), void (*action)(node *n, int arg), bool node_then_children)
{
		ptree_traverse_dfs_recursive(pt, filter, action, 0, node_then_children);
}

void ptree_traverse_dfs_recursive(node *pt, bool (*filter)(node *n), void (*action)(node *n, int depth), int depth, bool node_then_children)
{
	//static node *prev;

		if(node_then_children)
		{
			if((filter && filter(pt)) || (filter == NULL))
				action(pt, depth);
			//prev = pt;
		}

		//for(int ci=0; pt->children[ci]; ci++)
		for(int ci=0; ci<vector_len(pt->children); ci++)
		{
			ptree_traverse_dfs_recursive(pt->children[ci], filter, action, depth+1, node_then_children);
			//prev = &pt->children[ci];
		}

		if(!node_then_children)
		{
			if((filter && filter(pt)) || (filter == NULL))
				action(pt, depth);
			//prev = pt;
		}
}

//returns the first one that matches, or NULL
node *node_has_direct_child(node *pt, bool (*filter)(node *n))
{
	//for(int ci=0; pt->children[ci]; ci++)
	for(int ci=0; ci<vector_len(pt->children); ci++)
	{
		if(filter(pt->children[ci]))
			return pt->children[ci];
	}
	return NULL;
}

void node_print(node *pt, int depth)
{
	for(int i=0; i<depth; i++)
	{
		printf("  ");
	}
		if(pt->is_nonterminal == true)
			printf("(%s) ", gg.nonterminals[pt->type]);
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
