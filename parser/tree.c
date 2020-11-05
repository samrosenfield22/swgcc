

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

    n->children = vector(node *, 0);
    n->children[0] = NULL;

    if(str)
    {
    	/*n->str = malloc(strlen(str)+1);
    	assert(n->str);
    	strcpy(n->str, str);*/
    	n->str = strdup(str);
    }
    else
    	n->str = NULL;

    return n;
}

void node_add_child(node *root, node *child)
{
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

/*void ptree_do_to_each(node *n, void (*action)(node *n, int depth))
{
	node **collect = ptree_filter(n, NULL, -1);
	for(int i=0; i<vector_len(collect); i++)
		action(collect[i], );
	vector_destroy(collect);
}*/

node **ptree_filter(node *n, bool (*filter)(node *n), int depth)
{
	node **collect = vector(*collect, 0);
	ptree_filter_recursive(n, filter, collect, 0, depth);
	return collect;
}

void ptree_filter_recursive(node *n, bool (*filter)(node *n), node **collect, int depth, int max_depth)
{
	if((filter && filter(n)) || (filter == NULL))
	{
		vector_inc(&collect);
		vector_last(collect) = n;
	}

	if(depth >= max_depth && max_depth != -1)
		return;

	for(int i=0; i<vector_len(n->children); i++)
	{
		ptree_filter_recursive(n->children[i], filter, collect, depth+1, max_depth);
	}
}

node ref_node;	//declared extern in tree.h
bool filter_by_ref_node(node *n)
{
	if(n->is_nonterminal != ref_node.is_nonterminal)
		return false;

	if(n->is_nonterminal)
	{
		if(strcmp(gg.nonterminals[n->type], ref_node.str))
			return false;
	}
	else
	{
		if(n->type != ref_node.type)
			return false;
		if(n->str == NULL || strcmp(n->str, ref_node.str))
			return false;
	}

	return true;
}

//returns the first one that matches, or NULL
node *node_has_direct_child(node *pt, bool (*filter)(node *n))
{
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
