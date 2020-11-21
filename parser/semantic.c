

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "semantic.h"


//bool filter_assign_toks(node *n);

//void declare_new_vars(node *pt, int depth);
//void declare_new_vars(node *decl);

bool define_functions(bool *decl_only, node *pt);

void declare_new_vars(const char *typestr, node *mdecl);

bool is_lval(node *n);
bool is_lval_context_parent(node *n);
bool is_conditional(node *n);
node **get_lval_contexts(node *pt);


void amend_push_instr(node *pt, int arg, const char *instr);
void amend_lval(node *pt);
void amend_rval(node *pt);
void amend_num(node *pt);

//void update_jump_addr_pairs(node *loop);
void update_jump_semacts(node *loop, const char *jtype);

//void *get_zeroth_child(void *n);

//static node **get_nonterms(node *tree, char *ntstr);
static node *get_nonterm_child(node *parent, char *ntstr);
static node *get_nonterm_child_deep(node *parent, char *ntstr);
static bool is_nonterm_type(node *n, const char *ntstr);
static void semantic_print_failure(void);

enum semantic_status_type
{
	SEM_OK,
	SEM_REDECLARED_VAR,
	SEM_USING_UNDECLD_VAR,
	BAD_LVAL
} SEMANTIC_STATUS = SEM_OK;

#define SEMANTIC_BAIL_IF_NOT_OK	if(SEMANTIC_STATUS != SEM_OK) {semantic_print_failure(); return false;}


bool all_semantic_checks(bool *decl_only, node *pt)
{
	SEMANTIC_STATUS = SEM_OK;

	//tree_sanity_check(pt);

	if(!define_functions(decl_only, pt))				return false;
	if(!check_variable_declarations(pt))	return false;
	if(!handle_lvals(pt))					return false;
	if(!set_conditional_jumps(pt))			return false;
	//if(!resolve_tree_types(pt))				return false;

	return true;
}

void tree_sanity_check(node *pt)
{
	//node **invalid_ntypes = ptree_filter(pt, n->ntype > )
}

bool define_functions(bool *decl_only, node *pt)
{

	node **decls = ptree_filter(pt, is_nonterm_type(n, "decl"));
	vector_foreach(decls, i)
	{
		node **funcdef = ptree_filter(decls[i], is_nonterm_type(n, "funcdef"));
		if(vector_len(funcdef))
		{

			//there should only be 1
			assert(vector_len(funcdef) == 1);

			//update the base id's symbol, make it a function type
			node *bid = decls[i]->children[1];
			symbol *sym = bid->children[0]->sym;
			//printf("defining function \'%s\'\n", sym->name); getchar();
			assign_type_to_symbol(sym, "function");
			//sym->symtype = SYM_FUNCTION;
			sym->var = get_code_addr();
			sym->declared = true;

			*decl_only = true;

			vector_destroy(funcdef);
		}
	}
	vector_destroy(decls);

	return true;
}

bool check_variable_declarations(node *pt)
{
	//handle declarations for each sequence (comma or decl) separately
	//node **mstmts = get_nonterms(pt, "mstmt");
	node **mstmts = ptree_filter(pt, is_nonterm_type(n, "mstmt"));

	vector_foreach(mstmts, s)
	{
		//get all vars that should get declared
		/*node **mdecls = ptree_filter(mstmts[s], is_nonterm_type(n, "mdecl"));
		node **decl_ids = vector_map(mdecls, n->children[0], node *);	
		vector_destroy(mdecls);*/

		node **decl_parents = ptree_filter(mstmts[s],
			is_nonterm_type(n, "decl") || is_nonterm_type(n, "mdecl_assign"));
		vector_foreach(decl_parents, i)
		{
			if(get_nonterm_child_deep(decl_parents[i], "funcdef"))
			{
				printf("funcdef at child %d (len is %d)\n", i, vector_len(decl_parents));
				vector_delete(&decl_parents, i);
				i--;
			}
		}
		//node **decl_ids = vector_map(decl_parents, n->children[1], node *);
		node **decl_ids = vector_map(decl_parents, get_nonterm_child(n, "base_id"), node *);	
		vector_destroy(decl_parents);


		//get all vars that should already have been declared
		node **prev_decld_ids = ptree_filter(mstmts[s],		
			is_nonterm_type(n, "base_id") && vector_search(decl_ids, (int)n)==-1);

		/*printfcol(YELLOW_FONT, "decl ids:\n");
		vector_foreach(decl_ids, i) {printf("%s\n", decl_ids[i]->children[0]->str);}
		printfcol(YELLOW_FONT, "other ids:\n");
		vector_foreach(prev_decld_ids, i) {printf("%s\n", prev_decld_ids[i]->children[0]->str);}
		*/

		//check if all those vars are already declared
		vector_filter(prev_decld_ids, n->children[0]->sym->declared == false);

		vector_foreach(prev_decld_ids, i)
		{
			printfcol(RED_FONT, "undeclared variable %s\n", prev_decld_ids[i]->children[0]->str);
			if(!symbol_delete(prev_decld_ids[i]->children[0]->str))
				assert(0);
			SEMANTIC_STATUS = SEM_USING_UNDECLD_VAR;
		}
		SEMANTIC_BAIL_IF_NOT_OK

		//declare new variables, error for redeclared ones
		node *decl = get_nonterm_child(mstmts[s], "decl");
		if(decl)
		{
			char *type = decl->children[0]->str;	//decl->type->str
			vector_foreach(decl_ids, i)
				declare_new_vars(type, decl_ids[i]);
		}
		SEMANTIC_BAIL_IF_NOT_OK

		//clean up
		vector_destroy(decl_ids);
		//vector_destroy(all_bids);
		vector_destroy(prev_decld_ids);
	}

	vector_destroy(mstmts);
	return true;
}

bool handle_lvals(node *pt)
{

	//get all lvals and lval contexts
	node **lvals = ptree_filter(pt, is_lval(n));
	node **lval_contexts = get_lval_contexts(pt);

	node **lvals_in_context, **rvals_in_lval_context, **lvals_out_of_context;
	vector_intersect(&lvals_in_context, &lvals_out_of_context, &rvals_in_lval_context, lvals, lval_contexts);
	
	vector_foreach(rvals_in_lval_context, i)
	{
		set_text_color(RED_FONT);
		printf("unexpected expression in lval context ");
		node_print(rvals_in_lval_context[i], 0);
		printf("\n");
		SEMANTIC_STATUS = BAD_LVAL;
	}
	SEMANTIC_BAIL_IF_NOT_OK
	//if(vector_len(rvals_in_lval_context))
	//	return false;

	vector_foreach(lvals_in_context, i)
	{
		if(is_nonterm_type(lvals_in_context[i], "base_id"))
			amend_lval(lvals_in_context[i]);	//this only takes care of variables
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
		if(is_nonterm_type(lvals_out_of_context[i], "base_id"))
			amend_rval(lvals_out_of_context[i]);
	}


	//amend semacts for all int literals
	//node **int_lits = get_nonterms(pt, "base_num");
	node **int_lits = ptree_filter(pt, is_nonterm_type(n, "base_num"));
	vector_foreach(int_lits, i)
	{
		amend_num(int_lits[i]);
	}


	return true;
}

int jlpair;
bool set_conditional_jumps(node *pt)
{
	//swap the condition and stmtlist in while loops
	jlpair = 3;	//track jump addr/label pairs. this is a kludgey solution -- it can't be confused for any
				//addr/labels w id 0 or 1 (conditionals/loops) or 2 (function exit)


	//node **whiles = get_nonterms(pt, "while");
	node **whiles = ptree_filter(pt, is_nonterm_type(n, "while"));
	vector_foreach(whiles, i)
	{
		vector_swap(whiles[i]->children, 5, 8);	//swap the comma and stmtlist (nodes 5 and 8)
	}
	vector_destroy(whiles);

	//node **forloops = get_nonterms(pt, "forloop");
	node **forloops = ptree_filter(pt, is_nonterm_type(n, "forloop"));
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

	//node **all_conditionals = ptree_filter(pt, is_conditional, -1, false);	//get the deepest ones first
	node **all_conditionals = ptree_traverse_dfs(pt, is_conditional, NULL, -1, false);
	vector_foreach(all_conditionals, i)
	{
		//update_jump_addr_pairs(all_conditionals[i]);
		update_jump_semacts(all_conditionals[i], "pushaddr");
		update_jump_semacts(all_conditionals[i], "jumplabel");
		jlpair += 3;	//if we have multiple conditionals, this avoids them sharing jumplabel/pushaddr ids
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
	if(is_nonterm_type(n, "base_id"))		return true;
	if(is_nonterm_type(n, "base_expr"))		return is_lval(n->children[1]);	//the expr between the ()
	if(is_nonterm_type(n, "misc2_lval"))	return true;

	return false;
}

//&expr, ++expr/expr++, expr.n, expr = ..., expr += ...

/*				child #
misc2_context	1
misc1_context	0
assign 			0
decl 			1
mdecl_assign	1
*/
/*
an lval context parent is a node that is a parent of an lval context
in the production
<decl>			::= type <base_id> <declspec>?
<declspec>		::= <decl_assign> <mdecl_assign>*
<decl_assign>	::= "=" <assign> {=}
the base_id in the decl is in a lval context (it's being assign to), if the decl contains the declspec
(i.e. int a=5; a is an lval context because it's followed by an assignment)
*/
bool is_lval_context_parent(node *n)
{
	if(is_nonterm_type(n, "misc2_context"))		//&expr, ++/--expr
		return true;

	if(vector_len(n->children)==2)
	{
		if(is_nonterm_type(n, "misc1_context") ||	//expr++/--
			//is_nonterm_type(n, "mdecl") ||			//type id = ...
			is_nonterm_type(n, "assign"))			//expr = ..., expr += ...
			return true;
	}

	if(vector_len(n->children)==3 && is_nonterm_type(n, "decl"))
	{
		node *dspec = get_nonterm_child(n, "declspec");
		if(dspec && get_nonterm_child(dspec, "decl_assign"))
			return true;
	}

	if(vector_len(n->children)==4 && is_nonterm_type(n, "mdecl_assign"))	//type id = ..., id = ...
		if(get_nonterm_child(n, "decl_assign"))
			return true;

	return false;
}

bool is_conditional(node *n)
{
	return (is_nonterm_type(n, "if") || 
		is_nonterm_type(n, "while") || 
		is_nonterm_type(n, "dowhile") ||
		is_nonterm_type(n, "forloop") || 
		is_nonterm_type(n, "funcdef") );
}

//get the <base_id> inside the lval context (for each element of the vector)
//&expr, ++expr/expr++, expr.n, expr = ..., expr += ...
node **get_lval_contexts(node *pt)
{
	/*
	printf("all ids and terminal symbols:\n");
	node **flattened = ptree_filter(pt,
		is_nonterm_type(n, "base_id") || is_nonterm_type(n, "base_num") || (!(n->is_nonterminal) && n->ntype == TERMINAL));
	vector_foreach(flattened, i)
	{
		node_print(flattened[i], 0);
	}

	node **lval_contexts = vector(node *, 0);
	vector_foreach(flattened, i)
	{
		if(!is_nonterm_type(flattened[i], "base_id"))
			continue;

		//prepend lval contexts (&expr, ++expr)
		if(i && !flattened[i-1]->is_nonterminal && flattened[i-1]->ntype == TERMINAL)
		{
			if(strcmp(flattened[i-1]->str, "&")==0
				|| strcmp(flattened[i-1]->str, "++")==0
				|| strcmp(flattened[i-1]->str, "--")==0 )
				vector_append(lval_contexts, flattened[i]);
		}

		//append lval contexts (expr++, expr = ..., expr += ...)
		if(i < vector_len(flattened)-1 && !flattened[i+1]->is_nonterminal && flattened[i+1]->ntype == TERMINAL)
		{
			if(strcmp(flattened[i+1]->str, "++")==0
				|| strcmp(flattened[i+1]->str, "--")==0
				|| strcmp(flattened[i+1]->str, "=")==0
				|| strcmp(flattened[i+1]->str, "+=")==0
				|| strcmp(flattened[i+1]->str, "-=")==0
				|| strcmp(flattened[i+1]->str, "*=")==0
				|| strcmp(flattened[i+1]->str, "/=")==0)	//still need the rest of these (%=, etc)
				vector_append(lval_contexts, flattened[i]);
		}
	}
	return lval_contexts;

	exit(0);
	*/

	//node *child;
	node **context_parents = ptree_filter(pt, is_lval_context_parent(n));
	for(int i=0; i<vector_len(context_parents); i++)
	{
		if(is_nonterm_type(context_parents[i], "misc1_context") || is_nonterm_type(context_parents[i], "assign"))
			context_parents[i] = context_parents[i]->children[0];
		else if(is_nonterm_type(context_parents[i], "mdecl_assign"))
			context_parents[i] = context_parents[i]->children[2];
		else
			context_parents[i] = context_parents[i]->children[1];

		/*child = get_nonterm_child(context_parents[i], "base_id");
		if(!child)
			child = get_nonterm_child(context_parents[i], "base_expr");
		if(!child)
		{
			if(is_nonterm_type(context_parents[i], "misc2_context"))
				child = context_parents[i]->children[1];
			else if(is_nonterm_type(context_parents[i], "assign"))
				child = context_parents[i]->children[0];
			//working on this!!!
		}*/

		//step down through all the single-child nodes until we reach the bottom
		//(a base_id, or a base_expr)
		while(1)
		{
			if(vector_len(context_parents[i]->children) == 1)
				context_parents[i] = context_parents[i]->children[0];
			else if(is_nonterm_type(context_parents[i], "base_expr"))
				context_parents[i] = context_parents[i]->children[1];
			else
				break;
		}

		//context_parents[i] = child;
	}

	return context_parents;
}

//on
void declare_new_vars(const char *typestr, node *bid)
{
	assert(is_nonterm_type(bid, "base_id"));
	
	symbol *decl_sym = bid->children[0]->sym;
	if(decl_sym->declared)
	{
		SEMANTIC_STATUS = SEM_REDECLARED_VAR;
		return;
	}
	else
	{
		//declare and define the variable
		decl_sym->declared = true;
		assign_type_to_symbol(decl_sym, typestr);
		define_var(decl_sym);
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

void update_jump_semacts(node *loop, const char *jtype)
{
	assert(strcmp(jtype, "pushaddr")==0 || strcmp(jtype, "jumplabel")==0);

	char buf[41];
	node **jsemacts = ptree_filter(loop, !(n->is_nonterminal) && n->ntype==SEMACT && strstr(n->str, jtype));
	vector_foreach(jsemacts, i)
	{
		int jid = *(jsemacts[i]->str + strlen(jtype) + 1)-'0';	//+1 for the space after "pushaddr"/"jumplabel"
		snprintf(buf, 40, "%s %d", jtype, jid + jlpair);
		free(jsemacts[i]->str);
		jsemacts[i]->str = strdup(buf);
	}
	vector_destroy(jsemacts);
}

/*void update_jump_addr_pairs(node *loop)
{
	char buf[41];

	//not pairs anymore lol
	node **pushaddrs = ptree_filter(loop, !(n->is_nonterminal) && n->ntype==SEMACT && strstr(n->str, "pushaddr"));
	vector_foreach(pushaddrs, i)
	{
		int jid = *(pushaddrs[i]->str + strlen("pushaddr "))-'0';
		snprintf(buf, 40, "pushaddr %d", jid + jlpair);
		free(pushaddrs[i]->str);
		pushaddrs[i]->str = strdup(buf);
	}
	vector_destroy(pushaddrs);

	node **jlabels = ptree_filter(loop, !(n->is_nonterminal) && n->ntype==SEMACT && strstr(n->str, "jumplabel"));
	vector_foreach(jlabels, i)
	{
		int jid = *(jlabels[i]->str + strlen("jumplabel "))-'0';
		snprintf(buf, 40, "jumplabel %d", jid + jlpair);
		free(jlabels[i]->str);
		jlabels[i]->str = strdup(buf);
	}
	vector_destroy(jlabels);

	jlpair += 3;	//if we have multiple loops, we need to disambiguate between their labels
}*/

/*void *get_zeroth_child(void *n)
{
	node *nn = *(node **)n;
	if(vector_len(nn->children))
		return &(nn->children[0]);
	else
		return NULL;
}*/

/*static node **get_nonterms(node *tree, char *ntstr)
{
	ref_node = (node){.is_nonterminal=true, .ntype=0, .str=ntstr, .children=NULL, .sym=NULL};
	return ptree_filter(tree, filter_by_ref_node, -1, true);
}*/

//returns the first child that matches
static node *get_nonterm_child(node *parent, char *ntstr)
{
	vector_foreach(parent->children, i)
	{
		if(is_nonterm_type(parent->children[i], ntstr))
			return parent->children[i];
	}
	return NULL;
}

//looks all the way through the tree, returns the first nonterm match
static node *get_nonterm_child_deep(node *parent, char *ntstr)
{
	node **matches = ptree_filter(parent, is_nonterm_type(n, ntstr));
	node *child = vector_len(matches)? matches[0] : NULL;
	vector_destroy(matches);
	return child;
}

/*static node *get_semact_child(node *parent, char *ntstr)
{
	vector_foreach(parent->children, i)
	{
		node *c = parent->children[i];
		if(!c->is_nonterminal && c->ntype==SEMACT && strcmp(c->str, ntstr)==0)
			return parent->children[i];
	}
	return NULL;
}*/

static bool is_nonterm_type(node *n, const char *ntstr)
{
	//for testing only -- if i mistype the nonterm string, this will catch it
	bool ntmatch = false;
	vector_foreach(gg.nonterminals, i)
	{
		if(strcmp(gg.nonterminals[i], ntstr)==0)
		{
			ntmatch = true;
			break;
		}
	}
	if(!ntmatch)
	{
		printf("typo in nonterm name \"%s\" passed to is_nonterm_type (semantic.c)\n", ntstr);
		assert(0);
	}

	if(!(n->is_nonterminal))
		return false;
	return (strcmp(gg.nonterminals[n->ntype], ntstr)==0);
}


static void semantic_print_failure(void)
{
	const char *failure_messages[] =
	{
		[SEM_REDECLARED_VAR] 		= "attempting to redeclare variable",
		[SEM_USING_UNDECLD_VAR]		= "using undeclared variable(s)",
		[BAD_LVAL]					= "bad expression(s) in lvalue context"
	};

	set_text_color(RED_FONT);
	printf("error: ");
	puts(failure_messages[SEMANTIC_STATUS]);
	set_text_color(RESET_FONT);
}

