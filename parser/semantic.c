

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "semantic.h"


bool filter_assign_toks(node *n);

void declare_new_vars(node *pt, int depth);

void amend_push_instr(node *pt, int arg);
void amend_lval(node *pt);
void amend_rval(node *pt);
void amend_num(node *pt);

static void semantic_print_failure(void);

enum semantic_status_type
{
	SEM_OK,
	SEM_REDECLARED_VAR,
	SEM_USING_UNDECLD_VAR
} SEMANTIC_STATUS = SEM_OK;

#define SEMANTIC_BAIL_IF_NOT_OK	if(SEMANTIC_STATUS != SEM_OK) {semantic_print_failure(); return false;}

bool check_variable_declarations(node *pt)
{
	//declare new variables, error for redeclared ones
	SEMANTIC_STATUS = SEM_OK;
	ref_node = (node){.is_nonterminal=true, .type=0, .str="mdecl", .children=NULL, .sym=NULL};
	ptree_traverse_dfs(pt, filter_by_ref_node, declare_new_vars, true);
	SEMANTIC_BAIL_IF_NOT_OK


	node **flattened = ptree_filter(pt, filter_assign_toks, -1);
	for(int i=0; i<vector_len(flattened); i++)
	{
		//grab all base_ids
		ref_node = (node){.is_nonterminal=true, .type=0, .str="base_id", .children=NULL, .sym=NULL};
		if(filter_by_ref_node(flattened[i]))
		{
			//check for use of undeclared vars
			if(flattened[i]->children[0]->sym->declared == false)
			{
				SEMANTIC_STATUS = SEM_USING_UNDECLD_VAR;
				SEMANTIC_BAIL_IF_NOT_OK
			}

			//if the base_id is right before a "=", it's a lval
			ref_node = (node){.is_nonterminal=false, .type=TERMINAL, .str="=", .children=NULL, .sym=NULL};
			if(i+1<vector_len(flattened) && filter_by_ref_node(flattened[i+1]))
			{
				amend_lval(flattened[i]);
			}
			else
			{
				amend_rval(flattened[i]);
			}
		}
		
	}

	//update semacts for numbers
	ref_node = (node){.is_nonterminal=true, .type=0, .str="base_other", .children=NULL, .sym=NULL};
	for(int i=0; i<vector_len(flattened); i++)
	{
		if(filter_by_ref_node(flattened[i]))
			amend_num(flattened[i]);
	}
	

	return true;
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

void amend_push_instr(node *pt, int arg)
{
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
	if(pt->children[0]->type == TERMINAL)	//could be a "("
		return;
	int num = atoi(pt->children[0]->str);
	amend_push_instr(pt, num);
}


static void semantic_print_failure(void)
{
	const char *failure_messages[] =
	{
		[SEM_REDECLARED_VAR] 		= "attempting to redeclare variable",
		[SEM_USING_UNDECLD_VAR]		= "using undeclared variable"
	};

	puts(failure_messages[SEMANTIC_STATUS]);
}

