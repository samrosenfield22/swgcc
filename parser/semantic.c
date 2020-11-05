

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "semantic.h"

//void is_var_initialized(node *pt, int depth);

bool filter_node_mdecl(node *n);
bool filter_node_base_other(node *n);	//delete maybe
bool filter_node_base(node *n);
bool filter_lvalue_parents(node *n);

void check_var_decls(node *pt, int depth);
void amend_push_semact(node *pt, int depth);
void amend_push_lvalues(node *pt, int depth);

static void semantic_print_failure(void);

enum semantic_status_type
{
	SEM_OK,
	SEM_REDECLARED_VAR
} SEMANTIC_STATUS = SEM_OK;

//bool SEMANTIC_OK = true;

#define SEMANTIC_BAIL_IF_NOT_OK	if(SEMANTIC_STATUS != SEM_OK) {semantic_print_failure(); return false;}

bool check_variable_declarations(node *pt)
{
	//declare variables, error for redeclared ones
	SEMANTIC_STATUS = SEM_OK;
	//ptree_traverse_dfs(pt, filter_node_mdecl, check_var_decls, true);
	ref_node = (node){true, 0, NULL, "mdecl", NULL};
	ptree_traverse_dfs(pt, filter_by_ref_node, check_var_decls, true);
	SEMANTIC_BAIL_IF_NOT_OK

	//replace the "%s" in push semacts for lvalues with the variable addresses
	SEMANTIC_STATUS = SEM_OK;
	ptree_traverse_dfs(pt, filter_lvalue_parents, amend_push_lvalues, true);
	SEMANTIC_BAIL_IF_NOT_OK

	//replace the "%s" in push semacts with literals and variable names
	SEMANTIC_STATUS = SEM_OK;
	//ptree_traverse_dfs(pt, filter_node_base_other, amend_push_semact, true);
	//ptree_traverse_dfs(pt, filter_node_base, amend_push_semact, true);
	ref_node = (node){true, 0, NULL, "base_id", NULL};
	ptree_traverse_dfs(pt, filter_node_base, amend_push_semact, true);
	ref_node = (node){true, 0, NULL, "base_other", NULL};
	ptree_traverse_dfs(pt, filter_node_base, amend_push_semact, true);
	SEMANTIC_BAIL_IF_NOT_OK

	

	return true;
}

/*#define filter_nonterm(nt)		\
//	bool filter_node_##nt
*/

bool filter_node_mdecl(node *n)
{
	return (n->is_nonterminal && strcmp(gg.nonterminals[n->type], "mdecl")==0);
	//return (n->is_nonterminal && strcmp(n->str, "mdecl")==0);
}

bool filter_node_base_other(node *n)
{
	return (n->is_nonterminal && strcmp(gg.nonterminals[n->type], "base_other")==0);
}

bool filter_node_base(node *n)
{
	return (n->is_nonterminal &&
		(strcmp(gg.nonterminals[n->type], "base_other")==0 || strcmp(gg.nonterminals[n->type], "base_id")==0));
}

/*bool filter_lvalues(node *n)
{
	//the 0th child of any <assign> (IF it's a base_id)
	//the 0th child of any <mdecl>

	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "assign")==0)
	{
		if(n->children[0]->is_nonterminal && strcmp(gg.nonterminals[n->children[0]->type], "base_id")==0)
			return true;
	}

	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "mdecl")==0)
		return true;

	return false;
}*/

//assuming the lval semact must be child 0
bool filter_lvalue_parents(node *n)
{
	if(vector_len(n->children) < 2)
		return false;

	if(n->children[1]->is_nonterminal == false)
	{
		if(strcmp(n->children[1]->str, "lval")==0)
			return true;
	}
	return false;
}

//only gets called for mdecl nonterminals
void check_var_decls(node *pt, int depth)
{
	node *decl_var = pt->children[0]->children[0];	//child 0 is a base_id, its child 0 is the variable
														//n->0->0 is dependent on the specific grammar
	node_print(decl_var, 0);
	if(decl_var->sym->declared)
	{
		SEMANTIC_STATUS = SEM_REDECLARED_VAR;
		return;		//we'll keep scanning through the tree, but that's ok
	}
	else
	{
		//declare and define the variable
		decl_var->sym->declared = true;
		decl_var->sym->var = (int*)get_new_var(sizeof(int));

	}
}

//only gets called for base_other or base_id nonterminals
void amend_push_semact(node *pt, int depth)
{
	//if(filter_lvalues(pt))
	//	return;

	//copy the semact
	/*int fmt_len = strlen(pt->children[1]->str);
	char *buf = malloc(fmt_len+1);
	strcpy(buf, pt->children[1]->str);
	free(pt->children[1]->str);

	size_t new_len = strlen(pt->children[0]->str) + fmt_len + 1;
	pt->children[1]->str = malloc(new_len);

	sprintf(pt->children[1]->str, buf, pt->children[0]->str);
	free(buf);*/

	char *fmt = pt->children[1]->str;
	char *arg = pt->children[0]->str;

	if(!strstr(fmt, "%s"))
		return;

	//compose the ammended string
	int len = strlen(fmt) + strlen(arg) - 2;	//%s is 2 chars
	char *buf = malloc(len+1);
	assert(buf);
	sprintf(buf, fmt, arg);

	//copy it to the semact
	free(pt->children[1]->str);
	/*pt->children[1]->str = malloc(len+1);
	assert(pt->children[1]->str);
	strcpy(pt->children[1]->str, buf);*/
	pt->children[1]->str = strdup(buf);

	free(buf);
}

void amend_push_lvalues(node *pt, int depth)
{
	node *bidp = pt->children[0];	//base id is 0, {lval} is 1 (unless the grammar changes lol)

	char *fmt = bidp->children[1]->str;
	//char *arg = bidp->children[0]->str;
	int arg = (int)(bidp->children[0]->sym->var);

	if(!strstr(fmt, "%s"))
		return;

	//change the %s to a %d
	*(strstr(fmt, "%s")+1) = 'd';

	//printf("lvalue var: %s\naddr is: %p\n", bidp->children[0]->str, bidp->children[0]->sym->var);

	//printf("replacing \'%s\' with \'%s\'\n", )

	//compose the ammended string
	int len = strlen(fmt) + 8 - 2;	//int (the addr) will take up 8 chars, %s is 2 chars
	char *buf = malloc(len+1);
	assert(buf);
	sprintf(buf, fmt, arg);

	//copy it to the semact
	free(bidp->children[1]->str);
	/*bidp->children[1]->str = malloc(len+1);
	assert(bidp->children[1]->str);
	strcpy(bidp->children[1]->str, buf);*/
	bidp->children[1]->str = strdup(buf);

	free(buf);
}

static void semantic_print_failure(void)
{
	const char *failure_messages[] =
	{
		[SEM_REDECLARED_VAR] = "attempting to redeclare variable"
	};

	puts(failure_messages[SEMANTIC_STATUS]);
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