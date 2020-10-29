

#ifndef TREE_H_
#define TREE_H_

#include "recdesc_types.h"

#define NODE_CHILDREN_CT (20)

//parse tree node
typedef struct node_s node;
struct node_s
{
    //nonterminal_type type;
	bool is_nonterminal;
	int type;

    node *children[NODE_CHILDREN_CT];
    //node **next_child;
    int next_child;
    char *str;
};

extern const char *nt_strings[], *t_strings[];

node *node_create(bool is_nonterminal, int type, const char *str);
void node_add_child(node *root, node *child);

void ptree_traverse_dfs(node *pt, void (*action)(node *pt, int arg), bool node_then_children);
void ptree_traverse_dfs_recursive(node *pt, void (*action)(node *pt, int arg), int depth, bool node_then_children);

void node_print(node *pt, int depth);
void semact_print(node *pt, int depth);
void node_delete(node *pt, int dummy);

#endif //TREE_H_