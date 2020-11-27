

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "semantic.h"


//bool filter_assign_toks(node *n);

bool define_functions(bool *decl_only, node *pt);
int declare_new_vars(const char *typestr, node *mdecl, bool lifetime);

////////// that new ish
bool semantic_compiler_actions(node *pt);
bool add_decl_var(node *sem, node *var, node *dummy);
bool check_decl_parent(node *sem, node *id, node *dummy);
bool declare_vars(node *sem, node *dummy, node *dummy2);
bool alloc_lcls(node *sem, node *dummy, node *dummy2);
bool free_lcls(node *sem, node *dummy, node *dummy2);


bool is_lval(node *n);
bool is_lval_context_parent(node *n);
bool is_conditional(node *n);
node **get_lval_contexts(node *pt);


void amend_push_instr(node *pt, int arg, const char *instr);
void amend_lval(node *pt);
void amend_rval(node *pt);
void amend_num(node *pt);

void update_jump_semacts(node *loop, const char *jtype);

static node *get_nonterm_child(node *parent, char *ntstr);
static int get_nonterm_child_index(node *parent, char *ntstr);
static int get_semact_child_index(node *parent, char *str);
static bool is_semact_type(node *n, const char *sem);
static bool is_semact_special(node *n);
static node *get_nonterm_child_deep(node *parent, char *ntstr);
static bool is_nonterm_type(node *n, const char *ntstr);
static node *get_nonterm_ancestor(node *n, const char *ntstr);

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

	if(!define_functions(decl_only, pt))	return false;
	//if(!check_variable_declarations(pt))	return false;
	if(!semantic_compiler_actions(pt))		return false;
	if(!handle_lvals(pt))					return false;
	if(!set_conditional_jumps(pt))			return false;
	//if(!resolve_tree_types(pt))				return false;

	return true;
}

void tree_sanity_check(node *pt)
{
	//node **invalid_ntypes = ptree_filter(pt, n->ntype > )
}

typedef struct sem_compiler_act_s
{
	const char *str;
	int argc;
	bool (*action)(node *sem, node *arg0, node *arg1);	//dummy for actions w/o args
} sem_compiler_act;
sem_compiler_act COMPILER_ACTIONS[] =
{
	{"add_decl_var", 1, add_decl_var},
	{"check_decl_parent", 2, check_decl_parent},
	{"declare_vars", 0, declare_vars},

	{"alloc_lcls", 0, alloc_lcls},
	{"free_lcls", 0, free_lcls}

	//{"", 0, },
};

node **decl_var_list = NULL;//, **prev_decl_list;
bool semantic_compiler_actions(node *pt)
{
	if(!decl_var_list)
		decl_var_list = vector(*decl_var_list, 0);

	node **sem = ptree_filter(pt, is_semact_special(n));
	vector_foreach(sem, i)
	{
		bool special_action_recognized = false;
		for(int a=0; a<sizeof(COMPILER_ACTIONS)/sizeof(COMPILER_ACTIONS[0]); a++)
		{
			sem_compiler_act *aspec = &COMPILER_ACTIONS[a];
			char *match = strstr(sem[i]->str, aspec->str);
			if(match)
			{
				assert(match == &(sem[i]->str[2]));	//after the '!', ' '
				special_action_recognized = true;

				//get the arg(s)
				node *arg[2] = {NULL, NULL};
				//if(aspec->argc == 1)
				match += strlen(aspec->str) + 1;
				for(int ai=0; ai<aspec->argc; ai++)
				{
					

					//int sib_num = atoi(match);
					//arg = node_get_parent(sem[i])->children[sib_num];	//assumes we're getting the sibling

					//traverse the argument string (ex. parent.1 means get the parent, then get its child[1])
					arg[ai] = sem[i];
					while(1)
					{
						if(strstr(match, "parent")==match)	arg[ai] = node_get_parent(arg[ai]);
						else arg[ai] = arg[ai]->children[atoi(match)];

						while(!(*match=='.' || *match==',' || *match=='\0')) match++;

						//match = strchr(match, ',');
						
						if(*match == ',') {match+=2; break;}
						else if(*match == '\0') goto args_loaded;
						else if(*match == '.') match++;
						else assert(0);
					}

					//printf("arg %d\n", ai);
					//node_print(arg[ai], 0);
				}
				args_loaded:
				

				//call the function
				if(!aspec->action(sem[i], arg[0], arg[1]))
					return false;
			}
		}

		if(!special_action_recognized)
		{
			printfcol(RED_FONT, "unrecognized special semantic action: %s\n", sem[i]->str);
			assert(0);
		}

		node_delete_from_parent(sem[i]);
	}

	return true;
}

bool add_decl_var(node *sem, node *var, node *dummy)
{
	assert(is_nonterm_type(var, "base_id"));

	//for now, ignore it if it's a function
	node *parent = node_get_parent(var);
	if(get_nonterm_child_deep(parent, "funcdef"))
		return true;

	//printfcol(GREEN_FONT, "adding decl var to the list: %s\n", var->children[0]->str); //getchar();
	vector_append(decl_var_list, var);
	return true;

}

bool check_decl_parent(node *sem, node *id, node *parent)
{
	/*
		the node is inside a <decl>, it's already been added to the decl var list -- all good
		the node is not inside a <decl>, and the id is in the symbol table -- all good
		the node is not inside a <decl>, and the id is not in the symbol table -- using undecld variable!
	*/

	//if(get_nonterm_ancestor(id, "decl"))
	
	//node *parent = node_get_ancestor(id, 2);
	
	assert(parent == node_get_ancestor(id, 2));
	if(is_nonterm_type(parent, "decl") || is_nonterm_type(parent, "mdecl_assign"))
		return true;
	else
	{
		symbol *sym = symbol_search(id->str, SYM_IDENTIFIER);
		if(sym)
		{
			//add it to the prev_decl_list so it can get pointed to the symbol (when we're done w the stmt)
			//vector_append(prev_decl_list, id);
			id->sym = sym;
			//printf("\tattaching symbol to id %s\n", id->str);
			return true;
		}
		else
		{
			printfcol(RED_FONT, "undeclared variable %s\n", id->str);
			SEMANTIC_STATUS = SEM_USING_UNDECLD_VAR;
			return false;
		}

	}

	return true;
}

bool declare_vars(node *sem, node *dummy, node *dummy2)
{
	vector_foreach(decl_var_list, i)
	{
		node *id = decl_var_list[i]->children[0];
		id->sym = symbol_create(id->str, SYM_IDENTIFIER, NULL);

		node *decl = get_nonterm_ancestor(decl_var_list[i], "decl");
		char *type = decl->children[0]->str;	//decl->type->str

		//check if it's in a block
		node *containing_block = get_nonterm_ancestor(decl, "block");
		bool lifetime = containing_block? AUTO : STATIC;		//unless it's explicitly made static

		//printfcol(GREEN_FONT, "declaring var %s (%s %s)\n", id, (lifetime==AUTO)? "auto":"static", type);
		//getchar();

		int varsize = declare_new_vars(type, decl_var_list[i], lifetime);	//this 
		if(SEMANTIC_STATUS != SEM_OK)
		{
			return false;
		}

		if(containing_block)
			containing_block->block_bytes += varsize;
	}

	vector_destroy(decl_var_list);
	decl_var_list = vector(*decl_var_list, 0);
	return true;
}

bool alloc_lcls(node *sem, node *dummy, node *dummy2)
{
	char buf[21];
	node *parent = node_get_parent(sem);
	assert(is_nonterm_type(parent, "block"));
	size_t bytes = parent->block_bytes;

	snprintf(buf, 20, "incsp %d", bytes);
	free(sem->str);
	sem->str = strdup(buf);

	return true;
}

bool free_lcls(node *sem, node *dummy, node *dummy2)
{
	char buf[21];
	node *parent = node_get_parent(sem);
	assert(is_nonterm_type(parent, "block"));
	size_t bytes = parent->block_bytes;
	
	snprintf(buf, 20, "decsp %d", bytes);
	free(sem->str);
	sem->str = strdup(buf);

	return true;
}

////////////////////////////////////////////

bool define_functions(bool *decl_only, node *pt)
{

	//node **decls = ptree_filter(pt, is_nonterm_type(n, "decl"));
	//vector_foreach(decls, i)
	//{
		//node **funcdef = ptree_filter(decls[i], is_nonterm_type(n, "funcdef"));
		node **funcdef = ptree_filter(pt, is_nonterm_type(n, "funcdef"));
		if(vector_len(funcdef))
		{

			//there can only be 1
			assert(vector_len(funcdef) == 1);

			//update the base id's symbol, make it a function type
			//node *bid = decls[i]->children[1];
			node *decl = node_get_ancestor(funcdef[0], 2);
			assert(decl);
			node *bid = decl->children[1];

			//symbol *sym = bid->children[0]->sym;
			symbol *sym = bid->children[0]->sym = symbol_create(bid->children[0]->str, SYM_IDENTIFIER, NULL);
			assign_type_to_symbol(sym, "function");
			sym->var = get_code_addr();
			sym->declared = true;

			*decl_only = true;

			printfcol(GREEN_FONT, "defined function: %s\n", bid->children[0]->sym->name);
			//getchar();

			vector_destroy(funcdef);
		}
	//}
	//vector_destroy(decls);

	return true;
}


/*bool check_variable_declarations(node *pt)
{
	node **bids = ptree_filter(pt, is_nonterm_type(n, "base_id"));
	vector_foreach(bids, i)
	{
		//if it's in a declaration stmt, declare it
		//if not, make sure it's already declared
	}
}*/

bool check_variable_declarations(node *pt)
{
	//handle declarations for each sequence (comma or decl) separately
	//node **mstmts = ptree_filter(pt, is_nonterm_type(n, "mstmt") && !get_nonterm_ancestor(n, "mstmt"));
	node **mstmts = ptree_filter(pt, is_nonterm_type(n, "mstmt") && !get_nonterm_child_deep(n, "mstmt"));
	//node **blocks = ptree_filter(pt, is_nonterm_type(n, "block"));

	vector_foreach(mstmts, s)
	{
		//get all vars that should get declared
		//(all base_ids that are the targets of a decl statment, and are not function declarations)
		node **decl_parents = ptree_filter(mstmts[s],
			(is_nonterm_type(n, "decl") || is_nonterm_type(n, "mdecl_assign"))
			&& !get_nonterm_child_deep(n, "funcdef"));
		node **decl_ids = vector_map(decl_parents, get_nonterm_child(n, "base_id"));	
		vector_destroy(decl_parents);

		//get all vars that should already have been declared
		node **prev_decld_ids = ptree_filter(mstmts[s],		
			is_nonterm_type(n, "base_id") && vector_search(decl_ids, (int)n)==-1);

		/*set_text_color(YELLOW_FONT);
		printf("\ndeclaring variables for statement ");
		node_print(mstmts[s], 0);
		printf("vars that should get declared:");
		vector_foreach(decl_ids, i)
			node_print(decl_ids[i], 0);
		printf("vars that should already be declared:");
		vector_foreach(prev_decld_ids, i)
			node_print(prev_decld_ids[i], 0);
		set_text_color(RESET_FONT);

		vector_foreach(decl_ids, i)
		{
			vector_foreach(prev_decld_ids, j)
			{
				if(decl_ids[i] == prev_decld_ids[j])
				{
					printfcol(RED_FONT, "a m g e r y\n");
					exit(-1);
				}
			}
		}*/

		//check if all those vars are already declared
		vector_filter(prev_decld_ids, n->children[0]->sym->declared == false);
		vector_foreach(prev_decld_ids, i)
		{
			printfcol(RED_FONT, "undeclared variable %s\n", prev_decld_ids[i]->children[0]->str);
			if(!symbol_delete(prev_decld_ids[i]->children[0]->sym))
				assert(0);
			SEMANTIC_STATUS = SEM_USING_UNDECLD_VAR;
		}
		SEMANTIC_BAIL_IF_NOT_OK

		//declare new variables, error for redeclared ones
		node *decl = get_nonterm_child(mstmts[s], "decl");
		if(decl)
		{
			char *type = decl->children[0]->str;	//decl->type->str

			//check if it's in a block
			//bool lifetime = is_in_block? AUTO : STATIC;		//unless it's explicitly made static
			node *containing_block = get_nonterm_ancestor(decl, "block");
			bool lifetime = containing_block? AUTO : STATIC;		//unless it's explicitly made static
			

			int bytes_decld = 0;
			vector_foreach(decl_ids, i)
				bytes_decld += declare_new_vars(type, decl_ids[i], lifetime);

			//add code to the start/end of the block to alloc auto variables
			if(lifetime == AUTO)
			{
				char buf[21];

				vector_insert(&(containing_block->children), 1);
				snprintf(buf, 20, "incsp %d", bytes_decld);
				containing_block->children[1] = node_create(false, SEMACT, buf, NULL);

				int blkend = vector_len(containing_block->children)-1;
				vector_insert(&(containing_block->children), blkend);
				snprintf(buf, 20, "decsp %d", bytes_decld);
				containing_block->children[blkend] = node_create(false, SEMACT, buf, NULL);
			}
		}
		SEMANTIC_BAIL_IF_NOT_OK

		//clean up
		vector_destroy(decl_ids);
		//vector_destroy(all_bids);
		//vector_destroy(blocks);
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

void tree_swap_nodes(node *p1, int ind1, node *p2, int ind2)
{
	node *temp = p1->children[ind1];
	p1->children[ind1] = p2->children[ind2];
	p2->children[ind2] = temp;
}

int jlpair = 4;		//track jump addr/label pairs. this is a kludgey solution -- it can't be confused for any
					//addr/labels w id 0 or 1 (conditionals/loops) or 2 (function exit)

bool set_conditional_jumps(node *pt)
{
	//swap the condition and stmtlist in while loops
	node **whiles = ptree_filter(pt, is_nonterm_type(n, "while"));
	vector_foreach(whiles, i)
	{
		//vector_swap(whiles[i]->children, 5, 8);	//swap the comma and stmtlist (nodes 5 and 8)
		int fl_comma = get_nonterm_child_index(whiles[i], "comma");
		int fl_stmtlist = get_nonterm_child_index(whiles[i], "stmtlist");
		vector_swap(whiles[i]->children, fl_comma, fl_stmtlist);
	}
	vector_destroy(whiles);

	//swap the condition and stmtlist in for loops
	node **forloops = ptree_filter(pt, is_nonterm_type(n, "forloop"));
	vector_foreach(forloops, i)
	{
		int fl_comma = get_nonterm_child_index(forloops[i], "comma");
		int fl_stmtlist = get_nonterm_child_index(forloops[i], "stmtlist");
		vector_swap(forloops[i]->children, fl_comma, fl_stmtlist);
	}
	vector_destroy(forloops);

	//swap the base_id and pushaddr in function calls (we push the ret addr, THEN the function addr to jmp)
	node **fcalls = ptree_filter(pt,
		is_nonterm_type(n, "misc1_context") && get_nonterm_child(n, "base_id") && vector_len(n->children)==2);
	vector_filter(fcalls, vector_len(n->children[1]->children)>2);
	vector_foreach(fcalls, i)
	{
		node *fc = fcalls[i]->children[1];
		int paindex = get_semact_child_index(fc, "pushaddr 3");
		tree_swap_nodes(fcalls[i], 0, fc, paindex);
	}
	vector_destroy(fcalls);

	//node **all_conditionals = ptree_filter(pt, is_conditional, -1, false);	//get the deepest ones first
	node **all_conditionals = ptree_traverse_dfs(pt, is_conditional, NULL, -1, false);
	vector_foreach(all_conditionals, i)
	{
		update_jump_semacts(all_conditionals[i], "pushaddr");
		update_jump_semacts(all_conditionals[i], "jumplabel");
		jlpair += 4;	//if we have multiple conditionals, this avoids them sharing jumplabel/pushaddr ids
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

//really "is this a thing that has jump labels/addrs"
bool is_conditional(node *n)
{
	return (is_nonterm_type(n, "if") || 
		is_nonterm_type(n, "while") || 
		is_nonterm_type(n, "dowhile") ||
		is_nonterm_type(n, "forloop") || 
		is_nonterm_type(n, "funcdef") ||
		is_nonterm_type(n, "misc1_context") );	//func call
}

//get the <base_id> inside the lval context (for each element of the vector)
//&expr, ++expr/expr++, expr.n, expr = ..., expr += ...
node **get_lval_contexts(node *pt)
{
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
int declare_new_vars(const char *typestr, node *bid, bool lifetime)
{
	assert(is_nonterm_type(bid, "base_id"));
	
	symbol *decl_sym = bid->children[0]->sym;
	if(decl_sym->declared)
	{
		printfcol(RED_FONT, "redeclaring variable %s\n", bid->children[0]->str);
		SEMANTIC_STATUS = SEM_REDECLARED_VAR;
		return 0;
	}
	else
	{
		//declare and define the variable
		decl_sym->declared = true;
		decl_sym->lifetime = lifetime;
		assign_type_to_symbol(decl_sym, typestr);
		return define_var(decl_sym);
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
	bool lifetime = pt->children[0]->sym->lifetime;
	amend_push_instr(pt, (int)(pt->children[0]->sym->var), (lifetime==STATIC)? "push" : "pushl");
}

void amend_rval(node *pt)
{
	bool lifetime = pt->children[0]->sym->lifetime;
	amend_push_instr(pt, (int)(pt->children[0]->sym->var), (lifetime==STATIC)? "pushv" : "pushlv");
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
		//int jid = *(jsemacts[i]->str + strlen(jtype) + 1)-'0';	//+1 for the space after "pushaddr"/"jumplabel"
		int jid = atoi(jsemacts[i]->str + strlen(jtype) + 1);	//+1 for the space after "pushaddr"/"jumplabel"
		if(jid > 3)	//if the semact was already updated, skip it
			continue;
		snprintf(buf, 40, "%s %d", jtype, jid + jlpair);
		free(jsemacts[i]->str);
		jsemacts[i]->str = strdup(buf);
	}
	//printf("\tupdated %d %ss\n", vector_len(jsemacts), jtype);
	vector_destroy(jsemacts);
}

//returns the first child that matches
static node *get_nonterm_child(node *parent, char *ntstr)
{
	/*vector_foreach(parent->children, i)
	{
		if(is_nonterm_type(parent->children[i], ntstr))
			return parent->children[i];
	}
	return NULL;*/

	int index = get_nonterm_child_index(parent, ntstr);
	return (index==-1)? NULL : parent->children[index];
}

static int get_nonterm_child_index(node *parent, char *ntstr)
{
	vector_foreach(parent->children, i)
	{
		if(is_nonterm_type(parent->children[i], ntstr))
			return i;
	}
	return -1;
}

static int get_semact_child_index(node *parent, char *str)
{
	vector_foreach(parent->children, i)
	{
		//node *c = parent->children[i];
		//if(!c->is_nonterminal && c->ntype==SEMACT && strcmp(c->str, str)==0)
		//	return i;
		if(is_semact_type(parent->children[i], str))
			return i;
	}
	return -1;
}

static bool is_semact_type(node *n, const char *sem)
{
	if(n->is_nonterminal || n->ntype!=SEMACT)
		return false;
	return (strcmp(n->str, sem)==0);
}

static bool is_semact_special(node *n)
{
	if(n->is_nonterminal || n->ntype!=SEMACT)
		return false;
	return (n->str[0] == '!');
}

//looks all the way through the tree, returns the first nonterm match
static node *get_nonterm_child_deep(node *parent, char *ntstr)
{
	node **matches = ptree_filter(parent, is_nonterm_type(n, ntstr) && n!=parent);
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
	if(vector_search_str(gg.nonterminals, ntstr) == -1)
	{
		printf("typo in nonterm name \"%s\" passed to is_nonterm_type (semantic.c)\n", ntstr);
		assert(0);
	}

	if(!(n->is_nonterminal))
		return false;
	return (strcmp(gg.nonterminals[n->ntype], ntstr)==0);
}

static node *get_nonterm_ancestor(node *n, const char *ntstr)
{
	assert(n);
	/*printfcol(GREEN_FONT, "searching for nt %s\nnode: ", ntstr);
	node_print(n, 0);
	printf("\n");*/
	while(1)
	{
		n = node_get_parent(n);
		if(!n) break;

		/*printfcol(GREEN_FONT, "parent: ");
		node_print(n, 0);
		printf("\n");*/
		
		if(is_nonterm_type(n, ntstr))
			return n;
	}
	return NULL;
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

