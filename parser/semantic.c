

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "semantic.h"


//bool filter_assign_toks(node *n);

//void declare_new_vars(node *pt, int depth);
//void declare_new_vars(node *decl);
void declare_new_vars(const char *typestr, node *mdecl);

bool is_lval(node *n);
bool is_lval_context_parent(node *n);
bool is_conditional(node *n);
node **get_lval_contexts(node *pt);


void amend_push_instr(node *pt, int arg, const char *instr);
void amend_lval(node *pt);
void amend_rval(node *pt);
void amend_num(node *pt);

void update_jump_addr_pairs(node *loop);

//void *get_zeroth_child(void *n);

static node **get_nonterms(node *tree, char *ntstr);
static node *get_nonterm_child(node *parent, char *ntstr);
static bool is_nonterm_type(node *n, const char *ntstr);
static void semantic_print_failure(void);

enum semantic_status_type
{
	SEM_OK,
	SEM_REDECLARED_VAR,
	SEM_USING_UNDECLD_VAR
} SEMANTIC_STATUS = SEM_OK;

#define SEMANTIC_BAIL_IF_NOT_OK	if(SEMANTIC_STATUS != SEM_OK) {semantic_print_failure(); return false;}

int jlpair;
bool all_semantic_checks(node *pt)
{
	if(!check_variable_declarations(pt))	return false;
	if(!handle_lvals(pt))					return false;
	if(!set_conditional_jumps(pt))			return false;
	//if(!resolve_tree_types(pt))				return false;

	return true;
}



bool check_variable_declarations(node *pt)
{
	//handle declarations for each sequence (comma or decl) separately
	node **mstmts = get_nonterms(pt, "mstmt");

	vector_foreach(mstmts, s)
	{

		//get decl base_ids (vars that should get declared)
		node **mdecls = get_nonterms(mstmts[s], "mdecl");
		//node **decl_ids = vector_map(mdecls, get_zeroth_child);	//child 0 of a mdecl is a base_id
		node **decl_ids = vector_map(mdecls, n->children[0], node *);

		//make list of all base_ids
		node **all_bids = get_nonterms(mstmts[s], "base_id");

		//get the difference between the two vectors -- all ids that are not decl_ids
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
		node *decl = get_nonterm_child(mstmts[s], "decl");
		if(decl)
		{
			char *type = decl->children[0]->str;	//decl->type->str
			for(int i=0; i<vector_len(mdecls); i++)
			{
				declare_new_vars(type, mdecls[i]);
			}
			SEMANTIC_BAIL_IF_NOT_OK
		}

		//clean up
		vector_destroy(decl_ids);
		vector_destroy(all_bids);
		vector_destroy(mdecls);
		vector_destroy(prev_decld_ids);
	}

	vector_destroy(mstmts);
	return true;
}

bool handle_lvals(node *pt)
{

	//lvals
	//if it's in a lval context, but it's not an lval, error
	//if it's an lval, but it's not in a lval context
	node **lvals = ptree_filter(pt, is_lval, -1, true);
	node **lval_contexts = get_lval_contexts(pt);
	/*printf("lvals:\n");
	for(int i=0; i<vector_len(lvals); i++)
		node_print(lvals[i], 0);
	printf("\nlval contexts:\n");
	for(int i=0; i<vector_len(lval_contexts); i++)
	{
		node_print(lval_contexts[i], 0);
		printf("(%d children)\n", vector_len(lval_contexts[i]));
	}*/

	node **lvals_in_context, **rvals_in_lval_context, **lvals_out_of_context;
	vector_intersect(&lvals_in_context, &lvals_out_of_context, &rvals_in_lval_context, lvals, lval_contexts);
	//printf("rvals in lval context:\n");
	vector_foreach(rvals_in_lval_context, i)
	{
		printf("error: unexpected expression in lval context ");
		node_print(rvals_in_lval_context[i], 0);
		printf("\n");
	}
	if(vector_len(rvals_in_lval_context))
		return false;

	//printf("lvals in context:\n");
	vector_foreach(lvals_in_context, i)
	{
		//node_print(lvals_in_context[i], 0);
		//printf("\n");

		//if(strcmp(gg.nonterminals[lvals_in_context[i]->ntype], "base_id")==0)
		if(is_nonterm_type(lvals_in_context[i], "base_id"))
			amend_lval(lvals_in_context[i]);	//this only takes care of variables
		//else if(strcmp(gg.nonterminals[lvals_in_context[i]->ntype], "misc2_lval")==0)
		else if(is_nonterm_type(lvals_in_context[i], "misc2_lval"))
		{
			assert(lvals_in_context[i]->children[2]->ntype==SEMACT);
			vector_delete(&lvals_in_context[i]->children, 2);	//delete the '*' node
			//printf("--- lval in context that isn't a variable ---\n");
			//return 0;
		}
	}

	vector_foreach(lvals_out_of_context, i)
	{
		//if(strcmp(gg.nonterminals[lvals_out_of_context[i]->ntype], "base_id")==0)
		if(is_nonterm_type(lvals_out_of_context[i], "base_id"))
			amend_rval(lvals_out_of_context[i]);
	}


	//amend semacts for all int literals
	node **int_lits = get_nonterms(pt, "base_num");
	vector_foreach(int_lits, i)
	{
		amend_num(int_lits[i]);
	}


	return true;
}

bool set_conditional_jumps(node *pt)
{
	//swap the condition and stmtlist in while loops
	jlpair = 2;	//track jump addr/label pairs. this is a kludgey solution -- it can't be confused for any
				//addr/labels w id 0 or 1


	node **whiles = get_nonterms(pt, "while");
	vector_foreach(whiles, i)
	{
		vector_swap(whiles[i]->children, 5, 8);	//swap the comma and stmtlist (nodes 5 and 8)
	}
	vector_destroy(whiles);

	node **forloops = get_nonterms(pt, "forloop");
	vector_foreach(forloops, i)
	{
		//vector_swap(forloops[i]->children, 7, 12);	//swap the condition comma and stmtlist (nodes 7 and 12)
		int comma_i, stmtlist_i;
		vector_foreach(forloops[i]->children, j)
		{
			if(is_nonterm_type(forloops[i]->children[j], "comma"))		comma_i = j;
			if(is_nonterm_type(forloops[i]->children[j], "stmtlist"))	stmtlist_i = j;
		}
		vector_swap(forloops[i]->children, comma_i, stmtlist_i);
	}
	vector_destroy(forloops);

	node **all_conditionals = ptree_filter(pt, is_conditional, -1, false);	//get the deepest ones first
	vector_foreach(all_conditionals, i)
	{
		update_jump_addr_pairs(all_conditionals[i]);
	}
	vector_destroy(all_conditionals);

	return true;
}

/*bool resolve_tree_types(node *pt)
{

}*/

//vars, (lval), s.lval, s->a, *expr, a[n]
bool is_lval(node *n)
{
	/*if(n->is_nonterminal)
	{
		if(strcmp(gg.nonterminals[n->ntype], "base_id")==0)
			return true;
		if(strcmp(gg.nonterminals[n->ntype], "base_expr")==0)
			return (is_lval(n->children[1]));
		if(strcmp(gg.nonterminals[n->ntype], "misc2_lval")==0)
			return true;
	}*/
	if(is_nonterm_type(n, "base_id"))		return true;
	if(is_nonterm_type(n, "base_expr"))		return is_lval(n->children[1]);	//the expr between the ()
	if(is_nonterm_type(n, "misc2_lval"))	return true;

	return false;
}

//&expr, ++expr/expr++, expr.n, expr = ..., expr += ...
bool is_lval_context_parent(node *n)
{
	/*if(n->is_nonterminal)
	{
		if(strcmp(gg.nonterminals[n->ntype], "misc2_context")==0)	//&expr, ++/--expr
			return true;

		if(vector_len(n->children)==2 &&
		(
			strcmp(gg.nonterminals[n->ntype], "misc1_context")==0 ||	//expr++/--
			strcmp(gg.nonterminals[n->ntype], "mdecl")==0 ||			//type id = ...
			strcmp(gg.nonterminals[n->ntype], "assign")==0			//expr = ..., expr += ...
		))
			return true;
	}*/
	if(is_nonterm_type(n, "misc2_context"))		//&expr, ++/--expr
		return true;

	if(vector_len(n->children)==2)
	{
		if(is_nonterm_type(n, "misc1_context") ||	//expr++/--
			is_nonterm_type(n, "mdecl") ||			//type id = ...
			is_nonterm_type(n, "assign"))			//expr = ..., expr += ...
			return true;
	}

	return false;
}

bool is_conditional(node *n)
{
	/*if(n->is_nonterminal)
	{
		if(strcmp(gg.nonterminals[n->ntype], "if")==0 || 
			strcmp(gg.nonterminals[n->ntype], "while")==0 || 
			strcmp(gg.nonterminals[n->ntype], "dowhile")==0 || 
			strcmp(gg.nonterminals[n->ntype], "forloop")==0 )
			return true;
	}

	return false;
	*/
	return (is_nonterm_type(n, "if") || 
		is_nonterm_type(n, "while") || 
		is_nonterm_type(n, "dowhile") || 
		is_nonterm_type(n, "forloop") );
}


node **get_lval_contexts(node *pt)
{
	node **context_parents = ptree_filter(pt, is_lval_context_parent, -1, true);
	for(int i=0; i<vector_len(context_parents); i++)
	{
		/*if(strcmp(gg.nonterminals[context_parents[i]->ntype], "mdecl")==0 ||
			strcmp(gg.nonterminals[context_parents[i]->ntype], "assign")==0 ||
			strcmp(gg.nonterminals[context_parents[i]->ntype], "misc1_context")==0 )
			context_parents[i] = context_parents[i]->children[0];
		else //misc2_context
			context_parents[i] = context_parents[i]->children[1];*/
		if(is_nonterm_type(context_parents[i], "misc2_context"))
			context_parents[i] = context_parents[i]->children[1];
		else
			context_parents[i] = context_parents[i]->children[0];

		//step down through all the single-child nodes until we reach the bottom
		while(1)
		{
			if(vector_len(context_parents[i]->children) == 1)
				context_parents[i] = context_parents[i]->children[0];
			else if(is_nonterm_type(context_parents[i], "base_expr"))
				context_parents[i] = context_parents[i]->children[1];
			else
				break;
		}
	}

	//
	/*vector_foreach(context_parents, i)
	{
		while(1)
		{
			if(vector_len(context_parents[i]->children) == 1)
				context_parents[i] = context_parents[i]->children[0];
			else if(strcmp(gg.nonterminals[context_parents[i]->ntype], "base_expr")==0)
				context_parents[i] = context_parents[i]->children[1];
			else
				break;
		}
	}*/

	return context_parents;
}

//keep base_id, base_other, "=", more
/*bool filter_assign_toks(node *n)
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
}*/

//only gets called for mdecl nonterminals
//void declare_new_vars(node *pt, int depth)
void declare_new_vars(const char *typestr, node *mdecl)
{
	//node *decl_var = pt->children[0]->children[0];	//child 0 is a base_id, its child 0 is the variable
														//n->0->0 is dependent on the specific grammar
	//node *decl_var = pt->children[0];
	node *decl_var = mdecl->children[0]->children[0];	//decl->mdecl->base_id->id
	if(decl_var->sym->declared)
	{
		SEMANTIC_STATUS = SEM_REDECLARED_VAR;
		return;		//we'll keep scanning through the tree, but that's ok
	}
	else
	{
		//declare and define the variable
		decl_var->sym->declared = true;
		assign_type_to_symbol(decl_var->sym, typestr);
		//symbol *typesym = symbol_search(decl->children[0]->str, SYM_TYPESPEC);
		//decl_var->sym->type = typesym->type;
		//printf("creating new var of type %s (%d bytes)\n", decl->children[0]->str, typesym->type->bytes);
		//decl_var->sym->var = (int*)get_new_var(sizeof(int));
		//decl_var->sym->var = (int*)get_new_var(typesym->type->bytes);
		define_var(decl_var->sym);
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
	if(pt->children[0]->ntype == TERMINAL)	//could be a "("
		return;
	int num = atoi(pt->children[0]->str);
	amend_push_instr(pt, num, "push");
}

void update_jump_addr_pairs(node *loop)
{
	char buf[41];
	//vector_swap(whiles[i]->children, 5, 8);	//swap the comma and stmtlist (nodes 5 and 8)

	for(int i=0; i<2; i++)
	{
		//grab the matching pushaddr/label pair
		//we filter children-first order, so if there's a nested loop, we update the values for the deepest
		//one first (and avoid a scenario with multiple "pushaddr 0") 
		snprintf(buf, 40, "pushaddr %d", i);
		ref_node = (node){.is_nonterminal=false, .ntype=SEMACT, .str=buf, .children=NULL, .sym=NULL};
		node **pa = ptree_filter(loop, filter_by_ref_node, -1, false);

		snprintf(buf, 40, "jumplabel %d", i);
		ref_node = (node){.is_nonterminal=false, .ntype=SEMACT, .str=buf, .children=NULL, .sym=NULL};
		node **jl = ptree_filter(loop, filter_by_ref_node, -1, false);

		if(vector_len(pa) == 0)
			break;
		assert(vector_len(pa)==1);
		assert(vector_len(jl)==1);

		snprintf(buf, 40, "pushaddr %d", jlpair);
		free(pa[0]->str);
		pa[0]->str = strdup(buf);
		snprintf(buf, 40, "jumplabel %d", jlpair);
		free(jl[0]->str);
		jl[0]->str = strdup(buf);

		jlpair++;

		vector_destroy(pa);
		vector_destroy(jl);

	}
}

/*void *get_zeroth_child(void *n)
{
	node *nn = *(node **)n;
	if(vector_len(nn->children))
		return &(nn->children[0]);
	else
		return NULL;
}*/

static node **get_nonterms(node *tree, char *ntstr)
{
	ref_node = (node){.is_nonterminal=true, .ntype=0, .str=ntstr, .children=NULL, .sym=NULL};
	return ptree_filter(tree, filter_by_ref_node, -1, true);
}

//returns the first child that matches
static node *get_nonterm_child(node *parent, char *ntstr)
{
	vector_foreach(parent->children, i)
	{
		//if(strcmp(gg.nonterminals[parent->children[i]->ntype], ntstr)==0)
		if(is_nonterm_type(parent->children[i], ntstr))
			return parent->children[i];
	}
	return NULL;
}

static bool is_nonterm_type(node *n, const char *ntstr)
{
	if(!(n->is_nonterminal))
		return false;
	return (strcmp(gg.nonterminals[n->ntype], ntstr)==0);
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

