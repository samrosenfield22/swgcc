

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tree.h"

/*#ifdef NDEBUG
	#define tree_sanity_check(t)	//nothing
#endif	//NDEBUG*/

static node *node_create(size_t size, const char *str);
static void node_add_child(node *root, node *child);
//static node *node_has_direct_child(void *n, bool (*filter)(void *n));
static void node_delete(node *t, int dummy);

//
static void tree_print_pretty_rec(void *t, int depth, void (*print)(void *t));

void *tree_create(const char *str)
{
	node *n = node_create(sizeof(*n), str);
	n->parent = NULL;
	return n;
}

void *tree_create_extra(size_t size, const char *str)
{
	node *n = node_create(size, str);
    n->parent = NULL;
    return n;
}

void tree_add_child(void *root, void *child)
{
	//node *r=root;
	node_add_child(root, child);
	((node*)child)->parent = root;
}

void tree_insert_child(void *parent, void *child, int index)
{
	node *p=parent, *c=child;

	vector_insert(&(p->children), index);
	p->children[index] = c;
	c->parent = p;
}

//distance describes where the sibling is in relation to the node, i.e. 
//-2 means add the sibling 2 spaces to the left
void tree_add_sibling(void *n, void *sibl, int distance)
{
	int index = tree_get_parent_index(n) + distance;
	tree_insert_child(((node*)n)->parent, sibl, index);
}

//returns the node's index within its parent's "children" vector
int tree_get_parent_index(void *nn)
{
	node *n = (node*)nn;
	int index = vector_search(n->parent->children, (int)n);
	assert(index != -1);
	return index;
}

void *tree_get_parent(void *n)
{
	node *nn=n;
	return nn->parent;
}

void *tree_get_ancestor(void *n, size_t cnt)
{
	node *nn=n;
	for(int i=0; i<cnt; i++)
		if(nn)
			nn = nn->parent;
	return nn;
}

int *branch_list, *branch_ct;	//node lists for tracking branches during print_pretty

//if print is NULL, it just prints the node string
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
	const bool compress_tree = false;

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
	if(print)	print(t);
	else 		puts(tt->str);
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


////////////////////////////////////////////////
//dag

void *dag_create(const char *str)
{
	node *n = node_create(sizeof(*n), str);
	n->parents = vector(*(n->parents), 0);
	return n;
}

void *dag_create_extra(size_t size, const char *str)
{
	node *n = node_create(size, str);
    n->parents = vector(*(n->parents), 0);
    return n;
}


void dag_add_child(void *root, void *child)
{
	node_add_child(root, child);
	node **parents = ((node*)child)->parents;
	vector_append(parents, root);
}

//returns a vector containing all common parents to nodes in the input vector
void *dag_get_common_parents(void *d)
{
	node **dags = (node**)d;
	node **commons = vector(*commons, 0);

	vector_foreach(dags[0]->parents, i)
	{
		node *p = dags[0]->parents[i];
		bool found = true;
		for(int j=1; j<vector_len(dags); j++)
		{
			if(vector_search(dags[j]->parents, (int)p) == -1)
			{
				found = false;
				break;
			}
		}
		if(found)
			vector_append(commons, p);
	}

	return commons;
}



////////////////////////////////////////////////

//if we create a library that wraps this one (i.e. to make a node w extra elements), we can use this to allocate
//a node of given size (providing the extra elements come after the base node)
static node *node_create(size_t size, const char *str)
{
	node *n = malloc(size);
    assert(n);

    //n->parent = NULL;
    n->children = vector(node *, 0);
    n->str = strdup(str);
    return n;
}

static void node_add_child(node *root, node *child)
{
	vector_append(root->children, child);
	//c->parent = root;
}


//returns the first one that matches, or NULL
/*static node *node_has_direct_child(void *n, bool (*filter)(void *n))
{
	node *nn = (node*)n;
	for(int ci=0; ci<vector_len(nn->children); ci++)
	{
		if(filter(nn->children[ci]))
			return nn->children[ci];
	}
	return NULL;
}*/

static void node_delete(node *t, int dummy)
{
	//node *tt = (node*)t;
	vector_destroy(t->children);
	free(t);
}

//ok this one is really tree-specific but we'll leave it here for now
void node_delete_from_parent(void *n)
{
	node *nn = (node*)n;
	node *parent = tree_get_parent(nn);
	int index = vector_search(parent->children, (int)n);
	assert(index != -1);

	node_delete(n, 0);
	vector_delete(&(parent->children), index);
}
