

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "semantic.h"


bool filter_assign_toks(node *n);

void declare_new_vars(node *pt, int depth);

bool is_lval(node *n);
bool is_lval_context_parent(node *n);
node **get_lval_contexts(node *pt);


void amend_push_instr(node *pt, int arg, const char *instr);
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

bool all_semantic_checks(node *pt)
{
	if(!check_variable_declarations(pt))	return false;
	if(!handle_lvals(pt))	return false;

	//swap the condition and stmtlist in while loops
	int jlpair = 0;	//track jump addr/label pairs

	ref_node = (node){.is_nonterminal=true, .type=0, .str="while", .children=NULL, .sym=NULL};
	node **whiles = ptree_filter(pt, filter_by_ref_node, -1);
	vector_foreach(whiles, i)
	{
		char buf[41];
		vector_swap(whiles[i]->children, 5, 8);	//swap the comma and stmtlist (nodes 5 and 8)

		assert(strstr(whiles[i]->children[2]->str, "pushaddr"));
		assert(strstr(whiles[i]->children[7]->str, "jumplabel"));
		snprintf(buf, 40, "pushaddr %d", jlpair);
		free(whiles[i]->children[2]->str);
		whiles[i]->children[2]->str = strdup(buf);
		snprintf(buf, 40, "jumplabel %d", jlpair);
		free(whiles[i]->children[7]->str);
		whiles[i]->children[7]->str = strdup(buf);

		jlpair++;

		assert(strstr(whiles[i]->children[9]->str, "pushaddr"));
		assert(strstr(whiles[i]->children[4]->str, "jumplabel"));
		snprintf(buf, 40, "pushaddr %d", jlpair);
		free(whiles[i]->children[9]->str);
		whiles[i]->children[9]->str = strdup(buf);
		snprintf(buf, 40, "jumplabel %d", jlpair);
		free(whiles[i]->children[4]->str);
		whiles[i]->children[4]->str = strdup(buf);

		jlpair++;
	}
	vector_destroy(whiles);

	return true;
}

bool check_variable_declarations(node *pt)
{
	//handle declarations for each statement separately
	//this might be wrong for statements that contain other statements (ex. a if contains a stmtlist)
	ref_node = (node){.is_nonterminal=true, .type=0, .str="stmt", .children=NULL, .sym=NULL};
	node **stmts = ptree_filter(pt, filter_by_ref_node, -1);

	vector_foreach(stmts, s)
	{
		//make list of all decl base_ids (vars that are getting declared)
		ref_node = (node){.is_nonterminal=true, .type=0, .str="mdecl", .children=NULL, .sym=NULL};
		node **decl_ids = ptree_filter(stmts[s], filter_by_ref_node, -1);
		for(int i=0; i<vector_len(decl_ids); i++)
			decl_ids[i] = decl_ids[i]->children[0];

		//make list of all base_ids
		ref_node = (node){.is_nonterminal=true, .type=0, .str="base_id", .children=NULL, .sym=NULL};
		node **all_bids = ptree_filter(stmts[s], filter_by_ref_node, -1);

		node **prev_decld_ids;
		vector_intersect(NULL, &prev_decld_ids, NULL, all_bids, decl_ids);

		//check if all those vars are already declared
		for(int i=0; i<vector_len(prev_decld_ids); i++)
		{
			if(prev_decld_ids[i]->children[0]->sym->declared == false)
			{
				printf("undeclared variable %s\n", prev_decld_ids[i]->children[0]->str);
				return false;
			}
		}

		//declare new variables, error for redeclared ones
		SEMANTIC_STATUS = SEM_OK;
		//ref_node = (node){.is_nonterminal=true, .type=0, .str="mdecl", .children=NULL, .sym=NULL};
		//ptree_traverse_dfs(pt, filter_by_ref_node, declare_new_vars, true);
		for(int i=0; i<vector_len(decl_ids); i++)
			declare_new_vars(decl_ids[i], 0);
		SEMANTIC_BAIL_IF_NOT_OK

		//clean up
		vector_destroy(decl_ids);
		vector_destroy(all_bids);
		vector_destroy(prev_decld_ids);
	}

	
	
	return true;
}

bool handle_lvals(node *pt)
{

	//lvals
	//if it's in a lval context, but it's not an lval, error
	//if it's an lval, but it's not in a lval context
	node **lvals = ptree_filter(pt, is_lval, -1);
	node **lval_contexts = get_lval_contexts(pt);
	printf("lvals:\n");
	for(int i=0; i<vector_len(lvals); i++)
		node_print(lvals[i], 0);
	printf("\nlval contexts:\n");
	for(int i=0; i<vector_len(lval_contexts); i++)
	{
		node_print(lval_contexts[i], 0);
		printf("(%d children)\n", vector_len(lval_contexts[i]));
	}
	//exit(0);

	node **lvals_in_context, **rvals_in_lval_context, **lvals_out_of_context;
	vector_intersect(&lvals_in_context, &lvals_out_of_context, &rvals_in_lval_context, lvals, lval_contexts);
	printf("rvals in lval context:\n");
	vector_foreach(rvals_in_lval_context, i)
	{
		printf("error: unexpected expression in lval context ");
		node_print(rvals_in_lval_context[i], 0);
		printf("\n");
	}
	if(vector_len(rvals_in_lval_context))
		return false;

	printf("lvals in context:\n");
	vector_foreach(lvals_in_context, i)
	{
		node_print(lvals_in_context[i], 0);
		printf("\n");

		if(strcmp(gg.nonterminals[lvals_in_context[i]->type], "base_id")==0)
			amend_lval(lvals_in_context[i]);	//this only takes care of variables
		else if(strcmp(gg.nonterminals[lvals_in_context[i]->type], "misc2_lval")==0)
		{
			assert(lvals_in_context[i]->children[2]->type==SEMACT);
			vector_delete(&lvals_in_context[i]->children, 2);	//delete the '*' node
			//printf("--- lval in context that isn't a variable ---\n");
			//return 0;
		}
	}

	vector_foreach(lvals_out_of_context, i)
	{
		if(strcmp(gg.nonterminals[lvals_out_of_context[i]->type], "base_id")==0)
			amend_rval(lvals_out_of_context[i]);
	}


	//amend semacts for all int literals
	ref_node = (node){.is_nonterminal=true, .type=0, .str="base_num", .children=NULL, .sym=NULL};
	node **int_lits = ptree_filter(pt, filter_by_ref_node, -1);
	vector_foreach(int_lits, i)
	{
		amend_num(int_lits[i]);
	}


	return true;
}

//vars, (lval), s.lval, s->a, *expr, a[n]
bool is_lval(node *n)
{
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "base_id")==0)
		return true;
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "base_expr")==0)
		return (is_lval(n->children[1]));
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "misc2_lval")==0)
		return true;
	
	return false;
}

//&expr, ++expr/expr++, expr.n, expr = ..., expr += ...
bool is_lval_context_parent(node *n)
{
	//&expr, ++/--expr
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "misc2_context")==0)
		return true;

	//expr++/--
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "misc1_context")==0 &&
		vector_len(n->children) == 2)
		return true;

	//type id = ...
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "mdecl")==0 &&
		vector_len(n->children) == 2)
		return true;

	//expr = ..., expr += ...
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "assign")==0 &&
		vector_len(n->children) == 2)
		return true;

	return false;
}

node **get_lval_contexts(node *pt)
{
	node **context_parents = ptree_filter(pt, is_lval_context_parent, -1);
	for(int i=0; i<vector_len(context_parents); i++)
	{
		if(strcmp(gg.nonterminals[context_parents[i]->type], "mdecl")==0 ||
			strcmp(gg.nonterminals[context_parents[i]->type], "assign")==0 ||
			strcmp(gg.nonterminals[context_parents[i]->type], "misc1_context")==0 )
			context_parents[i] = context_parents[i]->children[0];
		else //misc2_context
			context_parents[i] = context_parents[i]->children[1];
	}

	//
	vector_foreach(context_parents, i)
	{
		while(1)
		{
			if(vector_len(context_parents[i]->children) == 1)
				context_parents[i] = context_parents[i]->children[0];
			else if(strcmp(gg.nonterminals[context_parents[i]->type], "base_expr")==0)
				context_parents[i] = context_parents[i]->children[1];
			else
				break;
		}
	}

	return context_parents;
}

//keep base_id, base_other, "=", more
bool filter_assign_toks(node *n)
{
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "base_id")==0)
		return true;
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "base_other")==0)
		return true;

	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "decl_assign")==0)
		return true;
	if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "massign")==0)
		return true;

	//if(n->is_nonterminal && strcmp(gg.nonterminals[n->type], "more")==0)
	//	return true;
	//if(!(n->is_nonterminal) && n->type==TERMINAL && strcmp(n->str, "=")==0)
	if(!(n->is_nonterminal) && n->type==TERMINAL)
		return true;

	return false;
}

//only gets called for mdecl nonterminals
void declare_new_vars(node *pt, int depth)
{
	//node *decl_var = pt->children[0]->children[0];	//child 0 is a base_id, its child 0 is the variable
														//n->0->0 is dependent on the specific grammar
	node *decl_var = pt->children[0];
	//node_print(decl_var, 0);
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

void amend_push_instr(node *pt, int arg, const char *instr)
{
	//compose the ammended string
	char buf[strlen(instr) + 12];	//2^32 has 10 digits, plus 1 for the space, 1 for '\0'
	sprintf(buf, "%s %d", instr, arg);

	//copy it to the semact
	free(pt->children[1]->str);
	pt->children[1]->str = strdup(buf);
}

void amend_lval(node *pt)
{
	amend_push_instr(pt, (int)(pt->children[0]->sym->var), "push");
}

void amend_rval(node *pt)
{
	amend_push_instr(pt, (int)(pt->children[0]->sym->var), "pushv");
}

void amend_num(node *pt)
{
	if(pt->children[0]->type == TERMINAL)	//could be a "("
		return;
	int num = atoi(pt->children[0]->str);
	amend_push_instr(pt, num, "push");
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

