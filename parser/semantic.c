

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "semantic.h"

//void is_var_initialized(node *pt, int depth);

bool filter_node_mdecl(node *n);
bool filter_node_base_other(node *n);

void check_var_decls(node *pt, int depth);
void amend_push_semact(node *pt, int depth);

bool SEMANTIC_OK = true;


bool check_variable_declarations(node *pt)
{
	//declare variables, error for redeclared ones
	SEMANTIC_OK = true;
	ptree_traverse_dfs(pt, filter_node_mdecl, check_var_decls, true);
	if(!SEMANTIC_OK) return false;

	//
	printf("\n\n");
	SEMANTIC_OK = true;
	ptree_traverse_dfs(pt, filter_node_base_other, amend_push_semact, true);
	if(!SEMANTIC_OK) return false;

	return SEMANTIC_OK;
}


bool filter_node_mdecl(node *n)
{
	return (n->is_nonterminal && strcmp(gg.nonterminals[n->type], "mdecl")==0);
}

bool filter_node_base_other(node *n)
{
	return (n->is_nonterminal && strcmp(gg.nonterminals[n->type], "base_other")==0);
}

//only gets called for mdecl nonterminals
void check_var_decls(node *pt, int depth)
{
	node *decl_var = pt->children[0]->children[0];	//child 0 is a base_id, its child 0 is the variable
														//n->0->0 is dependent on the specific grammar
	node_print(decl_var, 0);
	if(decl_var->sym->declared)
	{
		printf("declared already\n");
		SEMANTIC_OK = false;
		return;		//we'll keep scanning through the tree, but that's ok
	}
	else
	{
		printf("undecld\n");
		decl_var->sym->declared = true;
	}
}

//only gets called for base_other nonterminals
void amend_push_semact(node *pt, int depth)
{
	int fmt_len = strlen(pt->children[1]->str);
	char *buf = malloc(fmt_len+1);
	strcpy(buf, pt->children[1]->str);
	free(pt->children[1]->str);

	size_t new_len = strlen(pt->children[0]->str) + fmt_len + 1;
	pt->children[1]->str = malloc(new_len);

	sprintf(pt->children[1]->str, buf, pt->children[0]->str);
	free(buf);
}

//ptree_traverse_dfs(parse_tree, do_action_if_filter, true)


/*
if(node_has_parent(parse_tree, ))
*/
/*bool node_has_parent(node *root, node *n, int nonterm)
{
	//for each node nonterm within root,
		//check if that node has n as a child
	ptree_traverse_dfs(root, , true);
}

bool node_has_child(node *root, )
*/