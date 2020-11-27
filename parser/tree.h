

#ifndef TREE_H_
#define TREE_H_

#include "grammar.h"
#include "recdesc_types.h"
#include "../ds/vector.h"
#include "../utils/printcolor.h"

//parse tree node
typedef struct node_s node;
struct node_s
{
    //nonterminal_type type;
	bool is_nonterminal;
	int ntype;	//either a nonterminal index, or a type of thing (nonterminal, semantic, ident...)

	node *parent;
    node **children;
    char *str;

    //symbol *type;

    symbol *sym;	//if the node contains a variable, this points to its symbol in the sym table

    size_t block_bytes;	//only for "block" nonterms -- number of auto/local variable bytes
};

extern node ref_node;	//defined in tree.c
extern const char *t_strings[];

node *node_create(bool is_nonterminal, int ntype, const char *str, symbol *sym);
void node_add_child(node *root, node *child);
node *node_get_parent(node *n);
node *node_get_ancestor(node *n, size_t cnt);

//void ptree_init_names(char **strings);

//void ptree_traverse_dfs(node *pt, bool (*filter)(node *n), void (*action)(node *n, int arg), bool node_then_children);
//void ptree_traverse_dfs_recursive(node *pt, bool (*filter)(node *n), void (*action)(node *n, int depth), int depth, bool node_then_children);

//node **ptree_filter(node *n, bool (*filter)(node *n), int depth, bool node_then_children);
void ptree_action(node *n, void (*action)(node *n, int arg), bool node_then_children);

node **ptree_traverse_dfs
	(node *pt, bool (*filter)(node *n), void (*action)(node *n, int arg), int depth, bool node_then_children);
void ptree_traverse_dfs_recursive
	(node *pt, bool (*filter)(node *n), void (*action)(node *n, int depth),
	node ***collect, int depth, int max_depth, bool node_then_children);


//void ptree_filter_recursive(node *n, bool (*filter)(node *n), node ***collect, int depth, int max_depth);
bool filter_by_ref_node(node *n);

void ptree_print(node *pt);
void node_print(node *pt, int depth);
void tree_print_pretty(node *pt, int depth);
void node_print_str(node *pt, int depth);

void semact_print(node *pt, int depth);
void node_delete(node *pt, int dummy);
void node_delete_from_parent(node *n);

//"filter" is an expression where "n" refers to each node
#define ptree_filter(tree, filter)					\
({													\
	node **v = vector(*v, 0);						\
	node **stack = vector(*v, 0);					\
	vector_append(stack, tree);						\
	while(vector_len(stack))						\
	{												\
		node *n = vector_last(stack);							\
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