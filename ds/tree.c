

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tree.h"


//
static void tree_print_pretty_rec(void *t, int depth, void (*print)(void *t));


void *node_create(const char *str)
{
	return node_create_extra(sizeof(node), str);
}

//if we create a library that wraps this one (i.e. to make a node w extra elements), we can use this to allocate
//a node of given size (providing the extra elements come after the base node)
void *node_create_extra(size_t size, const char *str)
{
	node *n = malloc(size);
    assert(n);

    n->parent = NULL;
    n->children = vector(node *, 0);
    n->str = strdup(str);
    return n;
}

void node_add_child(void *root, void *child)
{
	node *r=root; node *c=child;

	vector_append(r->children, child);
	c->parent = root;
}

void *node_get_parent(void *n)
{
	node *nn=n;
	return nn->parent;
}

void *node_get_ancestor(void *n, size_t cnt)
{
	node *nn=n;
	for(int i=0; i<cnt; i++)
		if(nn)
			nn = nn->parent;
	return nn;
}


/*node **ptree_filter(node *n, bool (*filter)(node *n), int depth, bool node_then_children)
{
	return ptree_traverse_dfs(n, filter, NULL, -1, node_then_children);
}*/

void tree_action(void *n, void (*action)(void *n, int arg), bool node_then_children)
{
	tree_traverse_dfs(n, NULL, action, -1, node_then_children);
}

void **tree_traverse_dfs
	(void *t, bool (*filter)(void *n), void (*action)(void *n, int arg), int depth, bool node_then_children)
{
	void **collect = vector(*collect, 0);
	tree_traverse_dfs_recursive(t, filter, action, &collect, 0, depth, node_then_children);
	return collect;
}

void tree_traverse_dfs_recursive
	(void *t, bool (*filter)(void *n), void (*action)(void *n, int depth),
	void ***collect, int depth, int max_depth, bool node_then_children)
{
	if(node_then_children)
	{
		if((filter && filter(t)) || (filter == NULL))
		{
			if(action)
				action(t, depth);
			if(collect)
				vector_append((*collect), t);
		}
	}

	if(depth < max_depth || max_depth == -1)
		for(int ci=0; ci<vector_len(((node*)t)->children); ci++)
		{
			tree_traverse_dfs_recursive(((node*)t)->children[ci], filter, action, collect, depth+1, max_depth, node_then_children);
		}

	if(!node_then_children)
	{
		if((filter && filter(t)) || (filter == NULL))
		{
			if(action)
				action(t, depth);
			if(collect)
				vector_append((*collect), t);
		}
	}
}




//returns the first one that matches, or NULL
node *node_has_direct_child(void *n, bool (*filter)(void *n))
{
	node *nn = (node*)n;
	for(int ci=0; ci<vector_len(nn->children); ci++)
	{
		if(filter(nn->children[ci]))
			return nn->children[ci];
	}
	return NULL;
}


int *branch_list, *branch_ct;	//node lists for tracking branches during print_pretty

void tree_print_pretty(void *t, void (*print)(void *t))
{
	branch_list = vector(*branch_list, 0);
	branch_ct = vector(*branch_ct, 0);

	tree_print_pretty_rec(t, 0, print);

	vector_destroy(branch_list);
	vector_destroy(branch_ct);
}

static void tree_print_pretty_rec(void *t, int depth, void (*print)(void *t))
{
	node *tt = (node*)t;
	const bool compress_tree = true;

	//compress the tree by skipping nodes that only have 1 child
	if(compress_tree)
	{
		if(vector_len(tt->children) == 1)
		{
			tree_print_pretty_rec(tt->children[0], depth, print);
			return;
		}
	}

	if(tt->children && (vector_len(tt->children)>1))
	{
		vector_append(branch_list, depth-1);
		vector_append(branch_ct, vector_len(tt->children));
	}

	if(depth) printf(" ");
	for(int i=0; i<depth-1; i++)
	{
		if(vector_search(branch_list, i) == -1)
			printf("   ");
		else
			printf("  |");
	}

	int d = vector_search(branch_list, depth-2);
	if(depth)
	{
		if(d != -1)
			printf("--");
		else
			printf("  ");
	}

	//print the node
	//if(!pt->is_nonterminal && pt->ntype == SEMACT)		set_text_color(MAGENTA_FONT);
	print(t);	//depth??
	//if(!pt->is_nonterminal && pt->ntype == SEMACT)		set_text_color(RESET_FONT);

	if(d != -1)
	{
		branch_ct[d]--;
		if(branch_ct[d] == 0)
		{
			vector_delete(&branch_list, d);
			vector_delete(&branch_ct, d);
		}
	}

	//for(int i=0; i<vector_len(pt->children); i++)
	vector_foreach(tt->children, i)
		tree_print_pretty_rec(tt->children[i], depth+1, print);
}



void node_delete(void *t, int dummy)
{
	node *tt = (node*)t;
	vector_destroy(tt->children);
	free(t);
}

void node_delete_from_parent(void *n)
{
	node *nn = (node*)n;
	node *parent = node_get_parent(nn);
	int index = vector_search(parent->children, (int)n);
	assert(index != -1);

	node_delete(n, 0);
	vector_delete(&(parent->children), index);
}
