

#ifndef TREE_H_
#define TREE_H_

#include <stdbool.h>

#include "vector.h"

//tree node
typedef struct node_s node;
struct node_s
{
	node *parent;
    node **children;
    char *str;
};

void *node_create(const char *str);
void *node_create_extra(size_t size, const char *str);
void node_add_child(void *root, void *child);
void *node_get_parent(void *n);
void *node_get_ancestor(void *n, size_t cnt);

//void ptree_traverse_dfs(node *pt, bool (*filter)(node *n), void (*action)(node *n, int arg), bool node_then_children);
//void ptree_traverse_dfs_recursive(node *pt, bool (*filter)(node *n), void (*action)(node *n, int depth), int depth, bool node_then_children);

//node **ptree_filter(node *n, bool (*filter)(node *n), int depth, bool node_then_children);
void tree_action(void *n, void (*action)(void *n, int arg), bool node_then_children);

void **tree_traverse_dfs
	(void *pt, bool (*filter)(void *n), void (*action)(void *n, int arg), int depth, bool node_then_children);
void tree_traverse_dfs_recursive
	(void *pt, bool (*filter)(void *n), void (*action)(void *n, int depth),
	void ***collect, int depth, int max_depth, bool node_then_children);


//void ptree_filter_recursive(node *n, bool (*filter)(node *n), node ***collect, int depth, int max_depth);


void tree_print_pretty(void *t, void (*print)(void *t));

void node_delete(void *pt, int dummy);
void node_delete_from_parent(void *n);


//"filter" is an expression where "n" refers to each node
/*#define tree_filter(tree, filter)					\
({													\
	pnode **v = vector(*v, 0);						\
	pnode **stack = vector(*v, 0);					\
	vector_append(stack, tree);						\
	while(vector_len(stack))						\
	{												\
		pnode *n = vector_last(stack);							\
		vector_delete(&stack, vector_len(stack)-1);					\
													\
		if(filter) {vector_append(v, n);}			\
													\
		for(int i=vector_len(n->children)-1; i>=0; i--)					\
			vector_append(stack, n->children[i]);	\
	}												\
													\
	vector_destroy(stack);							\
	v;												\
})*/


//"filter" is an expression where "n" refers to each node
#define tree_filter(tree, filter)					\
({													\
	typeof(tree)* v = vector(*v, 0);						\
	typeof(tree)* stack = vector(*v, 0);					\
	vector_append(stack, tree);						\
	while(vector_len(stack))						\
	{												\
		typeof(tree) n = vector_last(stack);							\
		vector_delete(&stack, vector_len(stack)-1);					\
													\
		if(filter) {vector_append(v, n);}			\
													\
		for(int i=vector_len(n->children)-1; i>=0; i--)					\
			vector_append(stack, n->children[i]);	\
	}												\
													\
	vector_destroy(stack);							\
	v;												\
})

#endif //TREE_H_