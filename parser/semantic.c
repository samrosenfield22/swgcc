

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "semantic.h"


//bool filter_assign_toks(pnode *n);

bool define_functions(bool *decl_only, pnode *pt);
int declare_new_vars(const char *typestr, symbol *sym, bool lifetime, scopetype scope, pnode *block, bool is_arg);

////////// that new ish
bool semantic_compiler_actions(pnode *pt);
bool add_decl_var(pnode *sem, pnode *var, pnode *dummy);
bool check_decl_parent(pnode *sem, pnode *id, pnode *dummy);
bool declare_vars(pnode *sem, pnode *dummy, pnode *dummy2);
bool alloc_lcls(pnode *sem, pnode *dummy, pnode *dummy2);
bool free_lcls(pnode *sem, pnode *dummy, pnode *dummy2);
bool clean_args(pnode *sem, pnode *func_bid, pnode *dummy);
bool check_args(pnode *sem, pnode *funcid, pnode *arglist);
bool reverse_args(pnode *sem, pnode *arglist, pnode *dummy);
bool swap_nodes(pnode *sem, pnode *n1, pnode *n2);
bool define_function(pnode *sem, pnode *id, pnode *dummy);
bool handle_lvcontext(pnode *sem, pnode *id, pnode *dummy);
bool make_push(pnode *sem, pnode *bid, pnode *dummy);

void insert_local_allocations(pnode *sem, const char *instr, bool at_start);
pnode *get_containing_block(pnode *n);

bool is_lval(pnode *n);
bool is_conditional(void *n);

void update_jump_semacts(pnode *loop, const char *jtype);

static pnode *get_nonterm_child(pnode *parent, char *ntstr);
static int get_nonterm_child_index(pnode *parent, char *ntstr);
static int get_semact_child_index(pnode *parent, char *str);
static bool is_semact_type(pnode *n, const char *sem);
static bool is_semact_special(pnode *n);
//static pnode *get_nonterm_child_deep(pnode *parent, char *ntstr);
static bool is_nonterm_type(pnode *n, const char *ntstr);
static pnode *get_nonterm_ancestor(pnode *n, const char *ntstr);

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
	{0, "define_function", 2, define_function},

	{1, "add_decl_var", 1, add_decl_var},
	{1, "check_decl_parent", 2, check_decl_parent},
	{1, "declare_vars", 0, declare_vars},

	{1, "check_args", 2, check_args},
	{1, "reverse_args", 1, reverse_args},

	{2, "alloc_lcls", 0, alloc_lcls},
	{2, "free_lcls", 0, free_lcls},
	{2, "clean_args", 1, clean_args},

	{5, "swap_nodes", 2, swap_nodes},

	
	{3, "handle_lvcontext", 1, handle_lvcontext},
	{4, "make_push", 1, make_push}
	//{"", 0, },
};

#define MAX_ORDER 5
pnode **decl_var_list = NULL;	//list of id nodes to be declared
pnode **decl_func_list = NULL;	//list of function id nodes
bool semantic_compiler_actions(pnode *pt)
{
	if(decl_var_list) vector_destroy(decl_var_list);
	decl_var_list = vector(*decl_var_list, 0);
	if(decl_func_list) vector_destroy(decl_func_list);
	decl_func_list = vector(*decl_func_list, 0);

	pnode **sem = tree_filter(pt, is_semact_special(n), true);
	for(int ord=0; ord<=MAX_ORDER; ord++)
	{
		vector_foreach(sem, i)
		{
			for(int a=0; a<sizeof(COMPILER_ACTIONS)/sizeof(COMPILER_ACTIONS[0]); a++)
			{	
				sem_compiler_act *aspec = &COMPILER_ACTIONS[a];
				if(aspec->order != ord)
					continue;

				char *match = strstr(sem[i]->str, aspec->str);
				if(match)
				{
					assert(match == &(sem[i]->str[2]));	//after the '!', ' '

					//get the arg(s)
					pnode *arg[2] = {NULL, NULL};
					//if(aspec->argc == 1)
					match += strlen(aspec->str) + 1;
					for(int ai=0; ai<aspec->argc; ai++)
					{
						//traverse the argument string (ex. parent.1 means get the parent, then get its child[1])
						arg[ai] = sem[i];
						while(1)
						{
							if(strstr(match, "parent")==match)	arg[ai] = tree_get_parent(arg[ai]);
							else arg[ai] = arg[ai]->children[atoi(match)];

							//advance to the next delimiter
							while(!(*match=='.' || *match==',' || *match=='\0')) match++;
							if(*match == ',')
							{
								match++;
								if(*match == ' ') match++;
								break;
							}
							else if(*match == '\0') goto args_loaded;
							else if(*match == '.') match++;
							else assert(0);
						}
					}
					args_loaded:
					

					//call the function
					if(!aspec->action(sem[i], arg[0], arg[1]))
					{
						//semantic_print_failure();
						return false;
					}

					//
					//printf("deleting derived semact: %s\n", sem[i]->str);
					//node_delete_from_parent(sem[i]);
					vector_delete(&sem, i);
					i--;
					break;
				}
			}
		}
	}

	//i don't really want to delete nodes as we're going, as it could mess up the indexing for other semacts
	sem = tree_filter(pt, is_semact_special(n), true);
	vector_foreach(sem, i)
		node_delete_from_parent(sem[i]);

	return true;
}

bool add_decl_var(pnode *sem, pnode *var, pnode *dummy)
{
	//assert(is_nonterm_type(var, "base_id"));

	//for now, ignore it if it's a function. eventually we'l want to declare functions here
	//pnode *parent = tree_get_parent(var);
	//if(get_nonterm_child_deep(parent, "funcdef"))
	if(vector_search(decl_func_list, (int)var) != -1)
	{
		//printf("ignoring function var %s\n", var->str);
		return true;
	}

	//printfcol(GREEN_FONT, "adding decl var to the list: %s (%d)\n", var->str, (int)var); //getchar();
	vector_append(decl_var_list, var);
	return true;

}

//make sure each variable is not being used before being declared (either already decld or marked to be)
//also, if the var is already declared, assigns symbol to the id pnode
bool check_decl_parent(pnode *sem, pnode *id, pnode *parent)
{
	/*	the pnode is inside a <decl>, it's already been marked to be declared -- all good
		the pnode is not inside a <decl>, and the id is in the symbol table -- all good
		the pnode is not inside a <decl>, and the id is not in the symbol table -- using undecld variable!
		*/
	
	//assert(is_nonterm_type(id, "base_id"));
	assert(parent == tree_get_ancestor(id, 2));

	/*pnode *declspec = get_nonterm_child(parent, "declspec");
	if(declspec)
	{
		if(get_nonterm_child(declspec, "funcdef"))
		{
			printf("skipping decl check for function ")
			return true;
		}
	}*/
	if(vector_search(decl_func_list, (int)id) != -1)
	{
		return true;
	}

	//if(is_nonterm_type(parent, "decl") || is_nonterm_type(parent, "mdecl_assign"))
	//	return true;
	//printf("searching list of soon-to-be-declds for %d (%s)\n", (int)id, id->str);
	if(vector_search(decl_var_list, (int)id) != -1)
		return true;
	else
	{
		assert(id->sym == NULL);	//it's not declared so it shouldn't have a symbol

		/* get all blocks that are possible scopes for the variable -- that way if one shadows another,
		we get the one belonging to the innermost block
		this list must include NULL so we also look for a global variable */
		//pnode *b = get_nonterm_ancestor(id, "block");
		pnode *b = get_containing_block(id);
		pnode **blocks = vector(*blocks, 0);
		while(b)
		{
			vector_append(blocks, b);
			//b = get_nonterm_ancestor(b, "block");
			b = get_containing_block(b);
		}
		vector_append(blocks, NULL);

		//id->sym = symbol_search(id->str, SYM_IDENTIFIER);	//gets the one in the deepest block
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
		return get_nonterm_child(tree_get_parent(fdl), "block");
	//else if forloop
	else
		return get_nonterm_ancestor(n, "block");
}

//declare all vars in the list
bool declare_vars(pnode *sem, pnode *dummy, pnode *dummy2)
{
	//printf("declaring %d vars\n", vector_len(decl_var_list));
	vector_foreach(decl_var_list, i)
	{
		pnode *id = decl_var_list[i];
		//printfcol(GREEN_FONT, "declaring var %s\n", id->str);
		
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
		/*bool lifetime = containing_block? AUTO : STATIC;		//unless it's explicitly made static
		scopetype scope = containing_block? BLOCK : INTERNAL;	//unless extern
		bool is_arg = get_nonterm_ancestor(decl, "funcdeflist");*/

		//int varsize = declare_new_vars(type, id->sym, lifetime, scope, containing_block, is_arg);	//this 
		id->sym->declared = true;
		id->sym->lifetime = containing_block? AUTO : STATIC;	//unless explicitly made static
		id->sym->block = containing_block;
		id->sym->scope = containing_block? BLOCK : INTERNAL;	//unless extern
		id->sym->is_argument = get_nonterm_ancestor(id, "funcdeflist");
		
		assign_type_to_symbol(id->sym, type);
		int varsize = define_var(id->sym);

		//if(SEMANTIC_STATUS != SEM_OK)
		//	return false;

		/*printf("%d\n", containing_block);
		if(id->sym->is_argument)
		{
			//get the function (ancestor decl -> child base_id)
			pnode *fdecl = get_nonterm_ancestor(id, "decl");
			pnode *fcn = get_nonterm_child(fdecl, "base_id")->children[0];
			fcn->sym->argbytes += varsize;
			printf("adding %d argument bytes to function %s\n", varsize, fcn->str);
		}
		else */

		//block_bytes is for allocing/deallocing locals, not arguments
		if(containing_block && !(id->sym->is_argument))
		{
			containing_block->block_bytes += varsize;
		}

		//getchar();
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
	if(is_nonterm_type(arglist, "arglist"))	//optional nonterminal
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
	//pnode **temp = vector(*temp, vector_len(arglist->children));
	int argc = vector_len(arglist->children);
	pnode *temp[argc];

	vector_foreach(arglist->children, i)
	{
		temp[i] = arglist->children[argc-i-1];
	}

	memcpy(arglist->children, temp, argc * sizeof(pnode *));
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

bool clean_args(pnode *sem, pnode *func_id, pnode *dummy)
{
	//decsp pnode->argbytes

	//printf("after calling %s, clean %d bytes off the stack\n",
	//	func_id->str, func_id->sym->argbytes);
	//getchar();

	char buf[21];
	int argbytes = func_id->sym->argbytes;
	if(argbytes)
	{
		snprintf(buf, 20, "decsp %d", argbytes);

		pnode *newsem = pnode_create(false, SEMACT, buf, NULL);
		//tree_add_child(tree_get_parent(sem), newsem);
		pnode *parent = tree_get_parent(sem);
		int index = get_semact_child_index(parent, sem->str) + 1;	//so it doesn't get deleted (mistaken for the ! sem)

		//insert the semact pnode
		vector_insert(&parent->children, index);
		parent->children[index] = newsem;
		newsem->parent = parent;
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

		//insert the semact pnode
		vector_insert(&parent->children, index);
		parent->children[index] = newsem;
		newsem->parent = parent;
	}
}

bool swap_nodes(pnode *sem, pnode *n1, pnode *n2)
{
	/*printf("swapping nodes:\n");
	node_print(n1, 0);
	node_print(n2, 0);*/

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

bool define_function(pnode *sem, pnode *id, pnode *argdeflist)
{
	//id (decl->children[1]->children[0])
	id->sym = symbol_create(id->str, SYM_IDENTIFIER, NULL);
	assign_type_to_symbol(id->sym, "function");
	id->sym->var = get_code_addr();
	id->sym->declared = true;

	//get number of argument bytes
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
	//getchar();

	declaration_only = true;	//global

	vector_append(decl_func_list, id);

	//printfcol(GREEN_FONT, "defined function: %s\n", id->sym->name);
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

			//next one
			if(vector_len(n->children)==1) n = n->children[0];
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

	char instr[21], buf[21];
	int arg;
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
		arg = (int)id->sym->var;
		snprintf(instr, 20, "push%s%s", local? "l":"", (base->lval)? "":"v");
		/*if(local)
		{
			if(bid->lval)	strcpy(instr, "pushl");
			else			strcpy(instr, "pushlv");
		}
		else
		{
			if(bid->lval)	strcpy(instr, "push");
			else			strcpy(instr, "pushv");
		}*/
	}
	snprintf(buf, 20, "%s %d", instr, arg);

	pnode *pushact = pnode_create(false, SEMACT, buf, NULL);
	tree_add_child(base, pushact);
	return true;
}

////////////////////////////////////////////


int jlpair = 4;		//track jump addr/label pairs. this is a kludgey solution -- it can't be confused for any
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
		jlpair += 4;	//if we have multiple conditionals, this avoids them sharing jumplabel/pushaddr ids
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


//on
int declare_new_vars(const char *typestr, symbol *decl_sym, bool lifetime, scopetype scope, pnode *block, bool is_arg)
{
	/*assert(is_nonterm_type(bid, "base_id"));
	
	symbol *decl_sym = bid->children[0]->sym;
	if(decl_sym->declared)
	{
		printfcol(RED_FONT, "redeclaring variable %s\n", bid->children[0]->str);
		assert(0);
		SEMANTIC_STATUS = SEM_REDECLARED_VAR;
		return 0;
	}
	else
	{*/
		//declare and define the variable
		decl_sym->declared = true;
		decl_sym->lifetime = lifetime;
		decl_sym->block = block;
		decl_sym->scope = scope;
		decl_sym->is_argument = is_arg;
		if(is_arg)
		{
			//printf("declaring argument %s\n", )
		}
		assign_type_to_symbol(decl_sym, typestr);
		return define_var(decl_sym);
	//}
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
static pnode *get_nonterm_child(pnode *parent, char *ntstr)
{

	int index = get_nonterm_child_index(parent, ntstr);
	return (index==-1)? NULL : parent->children[index];
}

static int get_nonterm_child_index(pnode *parent, char *ntstr)
{
	vector_foreach(parent->children, i)
	{
		if(is_nonterm_type(parent->children[i], ntstr))
			return i;
	}
	return -1;
}

static int get_semact_child_index(pnode *parent, char *str)
{
	vector_foreach(parent->children, i)
	{
		//pnode *c = parent->children[i];
		//if(!c->is_nonterminal && c->ntype==SEMACT && strcmp(c->str, str)==0)
		//	return i;
		if(is_semact_type(parent->children[i], str))
			return i;
	}
	return -1;
}

static bool is_semact_type(pnode *n, const char *sem)
{
	if(n->is_nonterminal || n->ntype!=SEMACT)
		return false;
	return (strcmp(n->str, sem)==0);
}

static bool is_semact_special(pnode *n)
{
	if(n->is_nonterminal || n->ntype!=SEMACT)
		return false;
	return (n->str[0] == '!');
}

//looks all the way through the tree, returns the first nonterm match
/*static pnode *get_nonterm_child_deep(pnode *parent, char *ntstr)
{
	pnode **matches = tree_filter(parent, is_nonterm_type(n, ntstr) && n!=parent);
	pnode *child = vector_len(matches)? matches[0] : NULL;
	vector_destroy(matches);
	return child;
}*/

/*static pnode *get_semact_child(pnode *parent, char *ntstr)
{
	vector_foreach(parent->children, i)
	{
		pnode *c = parent->children[i];
		if(!c->is_nonterminal && c->ntype==SEMACT && strcmp(c->str, ntstr)==0)
			return parent->children[i];
	}
	return NULL;
}*/

static bool is_nonterm_type(pnode *n, const char *ntstr)
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

static pnode *get_nonterm_ancestor(pnode *n, const char *ntstr)
{
	assert(n);
	while(1)
	{
		n = tree_get_parent(n);
		if(!n) break;
		
		if(is_nonterm_type(n, ntstr))
			return n;
	}
	return NULL;
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

