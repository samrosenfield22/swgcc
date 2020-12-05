

#ifndef TREE_H_
#define TREE_H_

#include <stdbool.h>

#include "vector.h"

//tree node
typedef struct node_s node;
struct node_s
{
	union
	{
		node *parent;	//
		node **parents;	//
	};
    node **children;
    char *str;
};

void *tree_create(const char *str);
void *tree_create_extra(size_t size, const char *str);
void tree_add_child(void *root, void *child);
void *tree_get_parent(void *n);
void *tree_get_ancestor(void *n, size_t cnt);
void tree_print_pretty(void *t, void (*print)(void *t));

void node_delete_from_parent(void *n);


/*
me: "mom can we can some lambdas"
mom: "we have lambdas at home"
the lambdas:
*/
#define tree_filter(tree, filter, root_first)										\
({                             														\
	typeof(tree)* v = vector(*v, 0);           										\
	tree_traverse(tree, do {if(filter) vector_append(v, n);} while(0), root_first);	\
	if(!root_first) vector_reverse(v);                  							\
	v;                             													\
})

#define tree_traverse(tree, dothing, root_first)				\
({                             									\
	typeof(tree)* stack = vector(*stack, 0);  					\
	vector_push(stack, tree);                   				\
	while(!vector_is_empty(stack))             					\
	{                               							\
		typeof(tree) n = vector_pop(stack);     				\
		dothing;       											\
		                              							\
		void **children = vector_copy(((node*)n)->children);	\
		if(root_first)                           				\
			vector_reverse(children);							\
		vector_merge(&stack, children);                  		\
	}                               							\
	                              								\
	vector_destroy(stack);                   					\
                                                        		\
})

/*
	//test filter order
	void *root = tree_create("1");
    void *a = tree_create("2");
    void *b = tree_create("3");
    tree_add_child(root, a);
    tree_add_child(root, b);
    tree_add_child(a, tree_create("4"));
    tree_add_child(a, tree_create("5"));
    tree_add_child(b, tree_create("6"));
    tree_add_child(b, tree_create("7"));
    tree_add_child(b, tree_create("8"));
    tree_print_pretty(root, print_the_string);

    void **root_first = tree_filter(root, 1, true);
    vector_foreach(root_first, i) print_the_string(root_first[i]);

    printf("\n\n");

    void **children_first = tree_filter(root, 1, false);
    vector_foreach(children_first, i) print_the_string(children_first[i]);
*/

#endif //TREE_H_