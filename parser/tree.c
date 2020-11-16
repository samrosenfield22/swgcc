

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "tree.h"

char **nt_strings;

int *branch_list, *branch_ct;

const char *t_strings[] =
{
	[0] = "SHOULDNTSEETHIS",
	[IDENT] = "ident",
	[TERMINAL] = "terminal",
	[SEMACT] = "semact",
	[EXPR] = "expr"
};


node *node_create(bool is_nonterminal, int ntype, const char *str, symbol *sym)
{
    node *n = malloc(sizeof(*n));
    assert(n);

	n->is_nonterminal = is_nonterminal;
    n->ntype = ntype;
    n->sym = sym;
    //n->type = NULL;
    n->children = vector(node *, 0);
    n->str = strdup(str);

    return n;
}

void node_add_child(node *root, node *child)
{
	vector_append(root->children, child);
}

/*void ptree_init_names(char **strings)
{
	nt_strings = strings;
}*/

/*node **ptree_filter(node *n, bool (*filter)(node *n), int depth, bool node_then_children)
{
	return ptree_traverse_dfs(n, filter, NULL, -1, node_then_children);
}*/

void ptree_action(node *n, void (*action)(node *n, int arg), bool node_then_children)
{
	ptree_traverse_dfs(n, NULL, action, -1, node_then_children);
}

node **ptree_traverse_dfs
	(node *pt, bool (*filter)(node *n), void (*action)(node *n, int arg), int depth, bool node_then_children)
{
	node **collect = vector(*collect, 0);
	ptree_traverse_dfs_recursive(pt, filter, action, &collect, 0, depth, node_then_children);
	return collect;
}

void ptree_traverse_dfs_recursive
	(node *pt, bool (*filter)(node *n), void (*action)(node *n, int depth),
	node ***collect, int depth, int max_depth, bool node_then_children)
{
	if(node_then_children)
	{
		if((filter && filter(pt)) || (filter == NULL))
		{
			if(action)
				action(pt, depth);
			if(collect)
				vector_append((*collect), pt);
		}
	}

	if(depth < max_depth || max_depth == -1)
		for(int ci=0; ci<vector_len(pt->children); ci++)
		{
			ptree_traverse_dfs_recursive(pt->children[ci], filter, action, collect, depth+1, max_depth, node_then_children);
		}

	if(!node_then_children)
	{
		if((filter && filter(pt)) || (filter == NULL))
		{
			if(action)
				action(pt, depth);
			if(collect)
				vector_append((*collect), pt);
		}
	}
}

/*void ptree_do_to_each(node *n, void (*action)(node *n, int depth))
{
	node **collect = ptree_filter(n, NULL, -1);
	for(int i=0; i<vector_len(collect); i++)
		action(collect[i], );
	vector_destroy(collect);
}*/

node ref_node;	//declared extern in tree.h
bool filter_by_ref_node(node *n)
{
	if(n==NULL)
		return false;

	if(n->is_nonterminal != ref_node.is_nonterminal)
		return false;

	if(n->is_nonterminal)
	{
		if(strcmp(gg.nonterminals[n->ntype], ref_node.str))
			return false;
	}
	else
	{
		if(n->ntype != ref_node.ntype)
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

void ptree_print(node *pt)
{
	branch_list = vector(*branch_list, 0);
	branch_ct = vector(*branch_ct, 0);
	//ptree_traverse_dfs(pt, NULL, node_print_pretty, true);
	node_print_pretty(pt, 0);
	vector_destroy(branch_list);
	vector_destroy(branch_ct);
}

void node_print_pretty(node *pt, int depth)
{
	const bool compress_tree = true;

	//compress the tree by skipping nodes that only have 1 child
	if(compress_tree)
	{
		if(vector_len(pt->children) == 1)
		{
			node_print_pretty(pt->children[0], depth);
			return;
		}
	}

	if(pt->children && (vector_len(pt->children)>1))
	{
		vector_append(branch_list, depth-1);
		vector_append(branch_ct, vector_len(pt->children));
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

	if(!pt->is_nonterminal && pt->ntype == SEMACT)
		set_text_color(MAGENTA_FONT);
	node_print(pt, depth);
	if(!pt->is_nonterminal && pt->ntype == SEMACT)
		set_text_color(RESET_FONT);

	if(d != -1)
	{
		branch_ct[d]--;
		if(branch_ct[d] == 0)
		{
			vector_delete(&branch_list, d);
			vector_delete(&branch_ct, d);
		}
	}

	for(int i=0; i<vector_len(pt->children); i++)
		node_print_pretty(pt->children[i], depth+1);
}

void node_print(node *pt, int depth)
{
	bool is_semact = (!pt->is_nonterminal && pt->ntype == SEMACT);
	//if(is_semact)
	//	set_text_color(MAGENTA_FONT);

	//print node type
	if(pt->is_nonterminal == true)
		printf("(%s) ", gg.nonterminals[pt->ntype]);
	else
		printf("(%s) ", t_strings[pt->ntype]);

	//print node data
	//if(pt->str)
	//	printf("%s", pt->str);
	//if(!pt->is_nonterminal && pt->ntype == SEMACT)
	if(is_semact)
		printf("%s", pt->str);
	else
		ptree_action(pt, node_print_str, false);
		//ptree_traverse_dfs(pt, NULL, node_print_str, false);

	putchar('\n');

	//if(is_semact)
	//	set_text_color(RESET_FONT);
}

void node_print_str(node *pt, int depth)
{
	if(!(pt->is_nonterminal) && pt->ntype != SEMACT)
		if(pt->str)
			printf("%s ", pt->str);
}

void semact_print(node *pt, int depth)
{
	if(!(pt->is_nonterminal) && pt->ntype == SEMACT)
		puts(pt->str);
}

void node_delete(node *pt, int dummy)
{
	vector_destroy(pt->children);
	free(pt);
}
