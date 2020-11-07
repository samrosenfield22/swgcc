

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "semantic.h"

//void is_var_initialized(node *pt, int depth);

//bool filter_node_mdecl(node *n);
//bool filter_node_base_other(node *n);	//delete maybe
bool filter_node_base(node *n);
bool filter_lvalue_parents(node *n);
bool filter_assign_toks(node *n);

void declare_new_vars(node *pt, int depth);
void amend_push_semact(node *pt, int depth);
void amend_push_lvalues(node *pt, int depth);

void amend_push_instr(node *pt, int arg);
void amend_lval(node *pt);
void amend_rval(node *pt);
void amend_num(node *pt);

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
	//declare new variables, error for redeclared ones
	SEMANTIC_STATUS = SEM_OK;
	//ptree_traverse_dfs(pt, filter_node_mdecl, check_var_decls, true);
	ref_node = (node){true, 0, NULL, "mdecl", NULL};
	ptree_traverse_dfs(pt, filter_by_ref_node, declare_new_vars, true);
	SEMANTIC_BAIL_IF_NOT_OK

	//replace the "%s" in push semacts for lvalues with the variable addresses
	/*SEMANTIC_STATUS = SEM_OK;
	ptree_traverse_dfs(pt, filter_lvalue_parents, amend_push_lvalues, true);
	SEMANTIC_BAIL_IF_NOT_OK

	//replace the "%s" in push semacts with literals and variable names
	SEMANTIC_STATUS = SEM_OK;
	//ptree_traverse_dfs(pt, filter_node_base_other, amend_push_semact, true);
	//ptree_traverse_dfs(pt, filter_node_base, amend_push_semact, true);
	
	//ref_node = (node){.is_nonterminal=true, .type=0, .str="base_id", .children=NULL, .sym=NULL};
	ptree_traverse_dfs(pt, filter_node_base, amend_push_semact, true);
	//ref_node = (node){.is_nonterminal=true, .type=0, .str="base_other", .children=NULL, .sym=NULL};
	ptree_traverse_dfs(pt, filter_node_base, amend_push_semact, true);
	SEMANTIC_BAIL_IF_NOT_OK
	*/

	/*ref_node = (node){.is_nonterminal=true, .type=0, .str="mdecl", .children=NULL, .sym=NULL};
	node **decls = ptree_filter(root, filter_by_ref_node, -1);    //get all decl nodes
	for(int i=0; i<vector_len(decls); i++)
	{
	    for(int j=0; j<vector_len(decls[i]->children); j++)
	    {
	        if(decls[i]->children[j]->type==SEMACT && decls[i]->children[j]->str == "lval")
	        {
	            if(decls[i]->children[j-1] == "base_id") //direct left sibling of the = node
	                //do thing;
	        }
	    }
	}*/

	printf("--- flattened tree ---\n");
	node **flattened = ptree_filter(pt, filter_assign_toks, -1);
	for(int i=0; i<vector_len(flattened); i++)
	{
		//printf("%s ", (flattened[i]->str)? flattened[i]->str : gg.nonterminals[flattened[i]->type]);

		ref_node = (node){.is_nonterminal=true, .type=0, .str="base_id", .children=NULL, .sym=NULL};
		if(filter_by_ref_node(flattened[i]))
		{
			if(flattened[i]->children[0]->sym->declared == false)
			{
				printf("found undeclared var %s\n", flattened[i]->children[0]->str);
				return false;
			}

			//if the base_id is right before a "=", it's a lval
			ref_node = (node){.is_nonterminal=false, .type=TERMINAL, .str="=", .children=NULL, .sym=NULL};
			if(i+1<vector_len(flattened) && filter_by_ref_node(flattened[i+1]))
			{
				printf("lval %s\n", flattened[i]->children[0]->str);
				amend_lval(flattened[i]);
			}
			else
			{
				printf("rval %s\n", flattened[i]->children[0]->str);
				amend_rval(flattened[i]);
			}
		}
		

	}

	ref_node = (node){.is_nonterminal=true, .type=0, .str="base_other", .children=NULL, .sym=NULL};
	for(int i=0; i<vector_len(flattened); i++)
	{
		if(filter_by_ref_node(flattened[i]))
			amend_num(flattened[i]);
	}

	//exit(0);
	printf("\n----------------\n");
	

	return true;
}

/*#define filter_nonterm(nt)		\
//	bool filter_node_##nt
*/

/*bool filter_node_mdecl(node *n)
{
	return (n->is_nonterminal && strcmp(gg.nonterminals[n->type], "mdecl")==0);
	//return (n->is_nonterminal && strcmp(n->str, "mdecl")==0);
}

bool filter_node_base_other(node *n)
{
	return (n->is_nonterminal && strcmp(gg.nonterminals[n->type], "base_other")==0);
}
*/
bool filter_node_base(node *n)
{
	return (n->is_nonterminal &&
		(strcmp(gg.nonterminals[n->type], "base_other")==0 || strcmp(gg.nonterminals[n->type], "base_id")==0));
}

//keep base_id, "=", more
bool filter_assign_toks(node *n)
{
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "base_id")==0)
		return true;
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "base_other")==0)
		return true;
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "more")==0)
		return true;
	if(!(n->is_nonterminal) && n->type==TERMINAL && strcmp(n->str, "=")==0)
		return true;

	return false;
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
void declare_new_vars(node *pt, int depth)
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

	/*char *fmt = pt->children[1]->str;
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
	pt->children[1]->str = strdup(buf);

	free(buf);*/

	//char *op = pt->children[1]->str;
	char *arg = pt->children[0]->str;

	if(strcmp(pt->children[1]->str, "push"))
		assert(0);

	//if(!strstr(fmt, "%s"))
	//	return;

	//compose the ammended string
	int len = 5 + 10;	//5 for "push ", 2^32 has 10 digits
	char *buf = malloc(len+1);
	assert(buf);
	sprintf("push %d", arg);

	//copy it to the semact
	free(pt->children[1]->str);
	pt->children[1]->str = strdup(buf);

	free(buf);
}

void amend_push_instr(node *pt, int arg)
{
	//char *arg = pt->children[0]->str;

	//if(strcmp(pt->children[1]->str, "push"))
	//	assert(0);

	//compose the ammended string
	int len = 5 + 10;	//5 for "push ", 2^32 has 10 digits
	char *buf = malloc(len+1);
	assert(buf);
	sprintf(buf, "push %d", arg);

	//copy it to the semact
	free(pt->children[1]->str);
	pt->children[1]->str = strdup(buf);

	free(buf);
}

void amend_lval(node *pt)
{
	amend_push_instr(pt, (int)(pt->children[0]->sym->var));
}

void amend_rval(node *pt)
{
	amend_push_instr(pt, *(int*)(pt->children[0]->sym->var));
}

void amend_num(node *pt)
{
	int num = atoi(pt->children[0]->str);
	amend_push_instr(pt, num);
}

void amend_push_lvalues(node *pt, int depth)
{
	//node *bidp = pt->children[0];	//base id is 0, {lval} is 1 (unless the grammar changes lol)
	node *bidp = pt;

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