

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "semantic.h"



bool semantic_compiler_actions(pnode *pt);
bool add_decl_var(pnode *sem, pnode *var, pnode *dummy);
bool check_decl_var(pnode *sem, pnode *id, pnode *dummy);
bool seqpt(pnode *sem, pnode *dummy, pnode *dummy2);
bool alloc_lcls(pnode *sem, pnode *dummy, pnode *dummy2);
bool free_lcls(pnode *sem, pnode *dummy, pnode *dummy2);
bool clean_args(pnode *sem, pnode *func_bid, pnode *dummy);
bool check_args(pnode *sem, pnode *funcid, pnode *arglist);
bool reverse_args(pnode *sem, pnode *arglist, pnode *dummy);
bool define_functions(bool *decl_only, pnode *pt);
bool swap_nodes(pnode *sem, pnode *n1, pnode *n2);
bool define_function(pnode *sem, pnode *id, pnode *dummy);
bool handle_lvcontext(pnode *sem, pnode *id, pnode *dummy);
bool make_push(pnode *sem, pnode *bid, pnode *dummy);

void insert_local_allocations(pnode *sem, const char *instr, bool at_start);
pnode *get_containing_block(pnode *n);

bool is_lval(pnode *n);
bool is_conditional(void *n);

void update_jump_semacts(pnode *loop, const char *jtype);


//static void semantic_print_failure(void);

enum semantic_status_type
{
	SEM_OK,
	SEM_REDECLARED_VAR,
	SEM_USING_UNDECLD_VAR,
	BAD_LVAL,
	SEM_BAD_FUNC_CALL_ARGS
} SEMANTIC_STATUS = SEM_OK;

#define SEMANTIC_BAIL_IF_NOT_OK	if(SEMANTIC_STATUS != SEM_OK) {semantic_print_failure(); return false;}

bool declaration_only;
bool all_semantic_checks(bool *decl_only, pnode *pt)
{
	SEMANTIC_STATUS = SEM_OK;
	declaration_only = false;

	//tree_sanity_check(pt);

	//if(!define_functions(decl_only, pt))	return false;
	//if(!check_variable_declarations(pt))	return false;
	if(!semantic_compiler_actions(pt))		return false;
	//if(!handle_lvals(pt))					return false;
	if(!set_conditional_jumps(pt))			return false;
	//if(!resolve_tree_types(pt))				return false;

	*decl_only = declaration_only;
	return true;
}

void tree_sanity_check(pnode *pt)
{
	//pnode **invalid_ntypes = tree_filter(pt, n->ntype > )
}

typedef struct sem_compiler_act_s
{
	int order;		//order in which they get evaluated
	const char *str;
	int argc;
	bool (*action)(pnode *sem, pnode *arg0, pnode *arg1);	//dummy for actions w/o args
} sem_compiler_act;
sem_compiler_act COMPILER_ACTIONS[] =
{
	{0, "define_function", 1, define_function},

	{1, "add_decl_var", 1, add_decl_var},
	{1, "check_decl_var", 1, check_decl_var},
	{1, "seqpt", 0, seqpt},
	{1, "check_args", 2, check_args},
	{1, "reverse_args", 1, reverse_args},

	{2, "alloc_lcls", 0, alloc_lcls},
	{2, "free_lcls", 0, free_lcls},
	{2, "clean_args", 1, clean_args},
	
	{3, "handle_lvcontext", 1, handle_lvcontext},

	{4, "make_push", 1, make_push},

	{5, "swap_nodes", 2, swap_nodes}
	//{"", 0, },
};

//comparison function for qsort
/*int compar_semacts_by_order(const void *a, const void *b)
{
	pnode *an = (pnode *)a;
	pnode *bn = (pnode *)b;
	
	int aord = array_search(COMPILER_ACTIONS, ARRAY_LEN(COMPILER_ACTIONS), strstr(an->str, n->str));
	int bord = array_search(COMPILER_ACTIONS, ARRAY_LEN(COMPILER_ACTIONS), strstr(bn->str, n->str));
	
	assert(aord != -1);
	assert(bord != -1);

	return aord - bord;
}*/

#define MAX_ORDER 5
pnode **decl_var_list = NULL;	//list of id nodes to be declared
pnode **decl_func_list = NULL;	//list of function id nodes
bool semantic_compiler_actions(pnode *pt)
{
	if(decl_var_list) vector_destroy(decl_var_list);
	decl_var_list = vector(*decl_var_list, 0);
	if(decl_func_list) vector_destroy(decl_func_list);
	decl_func_list = vector(*decl_func_list, 0);

	sem_compiler_act *comp_actions = vector_from_arr(COMPILER_ACTIONS);

	pnode **sem = tree_filter(pt, is_semact_special(n), true);

	for(int ord=0; ord<=MAX_ORDER; ord++)
	{
		sem_compiler_act *comp_actions_order = vector_copy_filter(comp_actions, n.order == ord);

		vector_foreach(sem, i)
		{
			//get the compiler action that corresponds to the current semact
			int aindex = array_search(
				comp_actions_order, vector_len(comp_actions_order), strstr(sem[i]->str, n.str));
			if(aindex == -1) continue;	//wrong order
			sem_compiler_act *aspec = &comp_actions_order[aindex];

			//get the arg(s)
			pnode *arg[2] = {NULL, NULL};
			char *path = sem[i]->str + strlen(aspec->str) + 3;	//move past the "! " at the beginning, and the ' '
			char *end;

			for(int ai=0; ai<aspec->argc; ai++)
			{
				assert(path);
				end = strchr(path, ',');
				if(end)	*end = '\0';

				arg[ai] = ptree_walk_path(sem[i], path);

				if(!end) break;
				path = end+1;
				if(*path == ' ') path++;
			}

			//call the function
			if(!aspec->action(sem[i], arg[0], arg[1]))
			{
				//semantic_print_failure();
				return false;
			}

			//remove the semact from the list so we don't keep searching for it
			vector_delete(&sem, i);
			i--;
		}

		//reload the semacts in case one got deleted (this only works between semacts of different orders)
		vector_destroy(sem);
		sem = tree_filter(pt, is_semact_special(n), true);
	}

	//i don't really want to delete nodes as we're going, as it could mess up the indexing for other semacts
	sem = tree_filter(pt, is_semact_special(n), true);
	vector_foreach(sem, i)
		node_delete_from_parent(sem[i]);
	
	return true;
}

bool add_decl_var(pnode *sem, pnode *dcltor, pnode *dummy)
{
	//for now, ignore it if it's a function. eventually we'l want to declare functions here
	//pnode *parent = tree_get_parent(var);
	//if(get_nonterm_child_deep(parent, "funcdef"))
	
	assert(is_nonterm_type(dcltor, "full_dcltor"));
	pnode *id = get_nonterm_child_deep(dcltor, "base_id")->children[0];

	//printfcol(GREEN_FONT, "marking var for declaration: %s\n", id->str);

	if(vector_search(decl_func_list, (int)id) != -1)
	{
		return true;
	}

	vector_append(decl_var_list, id);
	return true;

}

//make sure each variable is not being used before being declared (either already decld or marked to be)
//also, if the var is already declared, assigns symbol to the id pnode
bool check_decl_var(pnode *sem, pnode *id, pnode *dummy)
{
	/*	the pnode is inside a <decl>, it's already been marked to be declared -- all good
		the pnode is not inside a <decl>, and the id is in the symbol table -- all good
		the pnode is not inside a <decl>, and the id is not in the symbol table -- using undecld variable!
		*/

	if(vector_search(decl_func_list, (int)id) != -1)
	{
		return true;
	}

	if(vector_search(decl_var_list, (int)id) != -1)
		return true;
	else
	{
		assert(id->sym == NULL);	//it's not declared so it shouldn't have a symbol

		/* get all blocks that are possible scopes for the variable -- that way if one shadows another,
		we get the one belonging to the innermost block
		this list must include NULL so we also look for a global variable */
		pnode *b = get_containing_block(id);
		pnode **blocks = vector(*blocks, 0);
		while(b)
		{
			vector_append(blocks, b);
			b = get_containing_block(b);
		}
		vector_append(blocks, NULL);
		id->sym = symbol_search_local(id->str, SYM_IDENTIFIER, (void**)blocks);
		vector_destroy(blocks);

		if(id->sym)
			return true;
		else
		{
			printfcol(RED_FONT, "undeclared variable %s\n", id->str);
			//dump_symbol_table();
			SEMANTIC_STATUS = SEM_USING_UNDECLD_VAR;
			return false;
		}
	}
}

//if the pnode is in the arg list of a function definition, this returns the function block
pnode *get_containing_block(pnode *n)
{
	pnode *fdl = get_nonterm_ancestor(n, "funcdeflist");
	if(fdl)
	{
		//return get_nonterm_child(tree_get_parent(fdl), "block");

		pnode *func_decl = get_nonterm_ancestor(n, "decl");
		return get_nonterm_child_deep(func_decl, "block");
	}
	//else if(get_nonterm_ancestor(n, "forloop"))
	else
		return get_nonterm_ancestor(n, "block");
}

//declare all vars in the list
bool seqpt(pnode *sem, pnode *dummy, pnode *dummy2)
{
	vector_foreach(decl_var_list, i)
	{
		pnode *id = decl_var_list[i];
		
		/* only look for a variable w matching name in the same scope! if there's a var (w same name)
		in an outer scope, we can shadow it */
		pnode **blocks = vector(*blocks, 1);
		pnode *containing_block = get_containing_block(id);
		blocks[0] = containing_block;
		bool var_same_name_in_scope = symbol_search_local(id->str, SYM_IDENTIFIER, (void**)blocks);
		vector_destroy(blocks);

		if(var_same_name_in_scope)
		{
			printfcol(RED_FONT, "redeclaring variable %s\n", id->str);
			SEMANTIC_STATUS = SEM_REDECLARED_VAR;
			return false;
		}
		id->sym = symbol_create(id->str, SYM_IDENTIFIER, NULL);

		//get variable attributes from the context
		pnode *decl = get_nonterm_ancestor(id, "funcdefarg");	//could use this to know if it's is_argument
		if(!decl)
			decl = get_nonterm_ancestor(id, "decl");
		char *type = decl->children[0]->str;	//decl->type->str

		//declare the variable
		id->sym->declared = true;
		id->sym->lifetime = containing_block? AUTO : STATIC;	//unless explicitly made static
		id->sym->block = containing_block;
		id->sym->scope = containing_block? BLOCK : INTERNAL;	//unless extern
		id->sym->is_argument = get_nonterm_ancestor(id, "funcdeflist");
		
		resolve_type(id->sym, get_nonterm_ancestor(id, "full_dcltor"), type);
		assign_type_to_symbol(id->sym, type);

		int varsize = define_var(id->sym);

		//if(SEMANTIC_STATUS != SEM_OK)
		//	return false;

		//block_bytes is for allocing/deallocing locals, not arguments
		if(containing_block && !(id->sym->is_argument))
		{
			containing_block->block_bytes += varsize;
		}
	}

	vector_destroy(decl_var_list);
	decl_var_list = vector(*decl_var_list, 0);
	//vector_destroy(decl_func_list);
	//decl_func_list = vector(*decl_func_list, 0);
	return true;
}

/*void function_dump_info(pnode *f)
{
	printf("func %s: arg bytes %d local bytes %d",
		f->str, f->sym->argbytes, get_nonterm_child_deep(f, "block")->block_bytes);
}*/

bool check_args(pnode *sem, pnode *funcid, pnode *arglist)
{
	//count args passed
	int passed = 0;
	//if(is_nonterm_type(arglist, "arglist"))	//optional nonterminal
	if(arglist)	//optional nonterminal
	{
		pnode **passed_args = tree_filter(arglist, is_nonterm_type(n, "margs"), true);	//args that come after a comma
		passed = vector_len(passed_args)+1;
		vector_destroy(passed_args);
	}

	int expected = funcid->sym->argct;
	if(passed != expected)
	{
		printfcol(RED_FONT, "error: invalid call to %s (expected %d args, got %d)\n",
			funcid->str, expected, passed);
		return false;
	}
	return true;
}

bool reverse_args(pnode *sem, pnode *arglist, pnode *dummy)
{
	vector_reverse(arglist->children);
	return true;
}

bool alloc_lcls(pnode *sem, pnode *dummy, pnode *dummy2)
{
	insert_local_allocations(sem, "incsp", true);
	return true;
}

bool free_lcls(pnode *sem, pnode *dummy, pnode *dummy2)
{
	insert_local_allocations(sem, "decsp", false);

	//remove symbols that are out of scope
	//pnode *block = get_nonterm_ancestor(sem, "block");
	//delete_locals_in_block(block);

	return true;
}

//clean args off the stack after returning
bool clean_args(pnode *sem, pnode *func_id, pnode *dummy)
{
	char buf[21];
	int argbytes = func_id->sym->argbytes;
	if(argbytes)
	{
		snprintf(buf, 20, "decsp %d", argbytes);

		pnode *newsem = pnode_create(false, SEMACT, buf, NULL);
		tree_add_sibling(sem, newsem, 1);
	}
	return true;
}

void insert_local_allocations(pnode *sem, const char *instr, bool at_start)
{
	char buf[21];
	pnode *parent = tree_get_parent(sem);
	assert(is_nonterm_type(parent, "block"));
	size_t bytes = parent->block_bytes;
	
	if(bytes)
	{
		snprintf(buf, 20, "%s %d", instr, bytes);
		pnode *newsem = pnode_create(false, SEMACT, buf, NULL);
		int index = at_start? 1 : vector_len(parent->children)-2;

		//insert the semact
		tree_insert_child(parent, newsem, index);
	}
}

//they can be in different vectors, so vector_swap doesn't do the trick
bool swap_nodes(pnode *sem, pnode *n1, pnode *n2)
{
	pnode *p1 = tree_get_parent(n1);
	int ind1 = vector_search(p1->children, (int)n1);
	assert(ind1 != -1);

	pnode *p2 = tree_get_parent(n2);
	int ind2 = vector_search(p2->children, (int)n2);
	assert(ind2 != -1);

	pnode *temp = n1;
	p1->children[ind1] = n2;
	p2->children[ind2] = temp;

	return true;
}

bool define_function(pnode *sem, pnode *dcltor, pnode *dummy)
{
	assert(is_nonterm_type(dcltor, "full_dcltor"));
	pnode *id = get_nonterm_child_deep(dcltor, "base_id")->children[0];
	//printfcol(GREEN_FONT, "\tdefining function: %s\n", id->str); getchar();

	//declare the function
	id->sym = symbol_create(id->str, SYM_IDENTIFIER, NULL);
	assign_type_to_symbol(id->sym, "function");
	id->sym->var = get_code_addr();
	id->sym->declared = true;

	//get number of argument bytes
	pnode *argdeflist = get_nonterm_child_deep(dcltor, "funcdeflist");
	pnode **defargs = tree_filter(argdeflist, is_nonterm_type(n, "funcdefarg"), true);
	pnode **types = vector_map(defargs, n->children[0]);
	size_t bytes = 0;
	vector_foreach(types, i)
	{
		symbol *type = symbol_search(types[i]->str, SYM_TYPESPEC);
		id->sym->argct++;
		bytes += type->tspec->bytes;
	}
	id->sym->argbytes = bytes;
	vector_destroy(defargs); vector_destroy(types);
	//printf("function %s has %d arg bytes\n", id->str, id->sym->argbytes);

	declaration_only = true;	//global

	vector_append(decl_func_list, id);

	//eliminate the push semacts for the function and its args (those are pushed by the caller,
	//not during definition)
	pnode **fbids = tree_filter(dcltor, is_nonterm_type(n, "base_id"), true);
	pnode **pushes = vector_map(fbids, get_semact_child(n, "! make_push parent"));
	vector_foreach(pushes, i) node_delete_from_parent(pushes[i]);
	vector_destroy(fbids); vector_destroy(pushes);

	return true;
}

//&expr, ++expr/expr++, expr.n, expr = ..., expr += ...
bool handle_lvcontext(pnode *sem, pnode *context, pnode *dummy)
{

	if(is_lval(context))
	{
		//mark the pnode as an lval, as well as relevant children (single-chain children and children in parens)
		for(pnode *n=context; ;)
		{
			n->lval = true;
			if(is_nonterm_type(n, "misc2_lval"))
			{
				assert(n->children[2]->ntype==SEMACT);
				vector_delete(&n->children, 2);	//delete the '*' pnode
			}

			//if the node is a full declarator, its id is an lval
			if(is_nonterm_type(n, "full_dcltor"))
				n = get_nonterm_child_deep(n, "base_id");

			//next one
			else if(vector_len(n->children)==1) n = n->children[0];
			//if(chldcnt == 1)						n = n->chil
			else if(is_nonterm_type(n, "base_expr")) n = n->children[1];
			else break;
		}
		return true;
	}
	else
	{
		//error
		set_text_color(RED_FONT);
		printf("unexpected expression in lval context: ");
		pnode_print(context);
		printf("\n");
		SEMANTIC_STATUS = BAD_LVAL;
		return false;
	}
}

bool make_push(pnode *sem, pnode *base, pnode *dummy)
{
	//bool local = get_nonterm_ancestor(bid, "block") &&
	//(is_nonterm_type(tree_get_parent(bid), "decl") || is_nonterm_type(tree_get_parent(bid), "mdecl_assign"));

	char instr[41], buf[41];
	long arg;
	pnode *id = base->children[0];

	//compose the push command string
	if(is_nonterm_type(base, "base_num"))
	{
		arg = atoi(id->str);
		strcpy(instr, "push");
	}
	else
	{
		bool local = id->sym->scope == BLOCK;
		arg = (long)id->sym->var;
		snprintf(instr, 40, "push%s%s", local? "l":"", (base->lval)? "":"v");
	}
	snprintf(buf, 40, "%s %ld", instr, arg);

	pnode *pushact = pnode_create(false, SEMACT, buf, NULL);
	tree_add_child(base, pushact);
	return true;
}

////////////////////////////////////////////

#define JMPLABEL_MAX (6)
int jlpair = JMPLABEL_MAX;		//track jump addr/label pairs. this is a kludgey solution -- it can't be confused for any
					//addr/labels w id 0 or 1 (conditionals/loops) or 2 (function exit)

bool set_conditional_jumps(pnode *pt)
{

	//pnode **all_conditionals = tree_filter(pt, is_conditional, -1, false);	//get the deepest ones first
	//pnode **all_conditionals = (pnode **)ptree_traverse_dfs(pt, is_conditional, NULL, -1, true);
	pnode **all_conditionals = tree_filter(pt, is_conditional(n), false);
	vector_foreach(all_conditionals, i)
	{
		update_jump_semacts(all_conditionals[i], "pushaddr");
		update_jump_semacts(all_conditionals[i], "jumplabel");
		jlpair += JMPLABEL_MAX;	//if we have multiple conditionals, this avoids them sharing jumplabel/pushaddr ids
	}
	vector_destroy(all_conditionals);

	return true;
}

/*bool resolve_tree_types(pnode *pt)
{

}*/

//vars, (lval), s.lval, s->a, *expr, a[n]
bool is_lval(pnode *n)
{
	if(is_nonterm_type(n, "base_id"))		return true;
	if(is_nonterm_type(n, "full_dcltor"))	return true;
	if(is_nonterm_type(n, "misc2_lval"))	return true;
	if(is_nonterm_type(n, "base_expr"))		return is_lval(n->children[1]);	//the expr between the ()
	if(vector_len(n->children)==1)			return is_lval(n->children[0]);

	return false;
}

//really "is this a thing that has jump labels/addrs"
bool is_conditional(void *n)
{
	return (is_nonterm_type(n, "if") || 
		is_nonterm_type(n, "while") || 
		is_nonterm_type(n, "dowhile") ||
		is_nonterm_type(n, "forloop") || 
		is_nonterm_type(n, "funcdef") ||
		is_nonterm_type(n, "misc1_context") );	//func call
}

void update_jump_semacts(pnode *loop, const char *jtype)
{
	assert(strcmp(jtype, "pushaddr")==0 || strcmp(jtype, "jumplabel")==0);

	char buf[41];
	pnode **jsemacts = tree_filter(loop, !(n->is_nonterminal) && n->ntype==SEMACT && strstr(n->str, jtype), true);
	vector_foreach(jsemacts, i)
	{
		//int jid = *(jsemacts[i]->str + strlen(jtype) + 1)-'0';	//+1 for the space after "pushaddr"/"jumplabel"
		int jid = atoi(jsemacts[i]->str + strlen(jtype) + 1);	//+1 for the space after "pushaddr"/"jumplabel"
		if(jid > JMPLABEL_MAX)	//if the semact was already updated, skip it
			continue;
		snprintf(buf, 40, "%s %d", jtype, jid + jlpair);
		free(jsemacts[i]->str);
		jsemacts[i]->str = strdup(buf);
	}
	//printf("\tupdated %d %ss\n", vector_len(jsemacts), jtype);
	vector_destroy(jsemacts);
}



/*static void semantic_print_failure(void)
{
	const char *failure_messages[] =
	{
		[SEM_REDECLARED_VAR] 		= "attempting to redeclare variable",
		[SEM_USING_UNDECLD_VAR]		= "using undeclared variable(s)",
		[BAD_LVAL]					= "bad expression(s) in lvalue context",
		[SEM_BAD_FUNC_CALL_ARGS]	= "bad function call"
	};

	set_text_color(RED_FONT);
	printf("error: ");
	puts(failure_messages[SEMANTIC_STATUS]);
	set_text_color(RESET_FONT);
}*/

