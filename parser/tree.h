

#ifndef TREE_H_
#define TREE_H_

#include "grammar.h"
#include "recdesc_types.h"
#include "../ds/vector.h"


//parse tree node
typedef struct node_s node;
struct node_s
{
    //nonterminal_type type;
	bool is_nonterminal;
	int type;	//either a nonterminal index, or a type of thing (nonterminal, semantic, ident...)

    node **children;
    char *str;

    symbol *sym;	//if the node contains a variable, this points to its symbol in the sym table
};

extern node ref_node;	//defined in tree.c
extern const char *t_strings[];

node *node_create(bool is_nonterminal, int type, const char *str, symbol *sym);
void node_add_child(node *root, node *child);

//void ptree_init_names(char **strings);

//void ptree_traverse_dfs(node *pt, void (*action)(node *pt, int arg), bool node_then_children);
//void ptree_traverse_dfs_recursive(node *pt, void (*action)(node *pt, int arg), int depth, bool node_then_children);

void ptree_traverse_dfs(node *pt, bool (*filter)(node *n), void (*action)(node *n, int arg), bool node_then_children);
void ptree_traverse_dfs_recursive(node *pt, bool (*filter)(node *n), void (*action)(node *n, int depth), int depth, bool node_then_children);

node **ptree_filter(node *n, bool (*filter)(node *n), int depth);
void ptree_filter_recursive(node *n, bool (*filter)(node *n), node ***collect, int depth, int max_depth);
bool filter_by_ref_node(node *n);

void ptree_print(node *pt);
void node_print(node *pt, int depth);
void node_print_pretty(node *pt, int depth);
void node_print_str(node *pt, int depth);

void semact_print(node *pt, int depth);
void node_delete(node *pt, int dummy);

#endif //TREE_H_