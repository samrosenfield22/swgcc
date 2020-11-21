

#include "nfa_run.h"

void *nfa_inst_stack;

typedef struct nfa_inst_s {int state; int sindex;} nfa_inst;

static void nfa_push_all_symbol_transitions(nfa_model *n, int state, char sym, int next_index);
static void *nfa_get_symbol_transitions(nfa_model *n, int state, char sym);
static int **lookup_trans_col_by_symbol(nfa_model *n, char sym);

void nfa_simulator_initialize(void)
{
	nfa_inst_stack = stack_create(256, nfa_inst);
}




//returns the greatest number of moves taken to reach the accept state
//or -1 if the accept state wasn't reached
int nfa_run(nfa_model *n, const char *s)
{
	/*
	(state, sindex) stores all the info for a given nfa instance

	clear/initialize the stack
	push (start,0) onto stack
	while stack is nonempty

		ni = pop from stack		//Nfa Instance

		if we're in an accept state and we're at the end of the string, return true

		t = nfa_transition()

	return false
	*/

	int best_moves = -1;

	stack_clear(nfa_inst_stack);
	stack_push(nfa_inst_stack, &(nfa_inst){n->start_state, 0});
	while(!stack_is_empty(nfa_inst_stack))
	{
		//pop the last nfa instance off the stack
		nfa_inst *ni = stack_pop(nfa_inst_stack);
		int cur_state = ni->state;
		char cur_char = s[ni->sindex];

		//check if we accept this instance
		//if(nfa_is_accept_state(n, cur_state) && cur_char=='\0')
		//	return true;

		//if this instance made it to an accept state, check how many moves it took to get there
		if(nfa_is_accept_state(n, cur_state))
		{
			int moves = ni->sindex;
			if((moves > best_moves) || (moves == best_moves && cur_char == '\0'))
			{
				best_moves = moves;
			}
		}

		//make all possible transitions (with the next string char, or empty). push them onto the stack.
		//if there are no transitions, we don't need to do anything, the loop goes ahead to the next instance
		nfa_transition(n, cur_state, cur_char, ni->sindex);
	}

	//return false;
	return best_moves;
}

//push all possible transitions onto the stack
void nfa_transition(nfa_model *n, int state, char c, int sindex)
{
	nfa_push_all_symbol_transitions(n, state, EMPTY_SYMBOL, sindex);
	nfa_push_all_symbol_transitions(n, state, c, sindex+1);
}

bool nfa_is_accept_state(nfa_model *n, int state)
{
	for(int *ap = n->accept_states; *ap != -1; ap++)
	{
		if(*ap == state)
			return true;
	}
	return false;
}

static void nfa_push_all_symbol_transitions(nfa_model *n, int state, char sym, int next_index)
{
	int *tr = nfa_get_symbol_transitions(n, state, sym);
	if(tr != NULL)
	{
		for(; *tr != -1; tr++)
			stack_push(nfa_inst_stack, &(nfa_inst){*tr, next_index});	//empty transition, so index stays the same
	}
}

static void *nfa_get_symbol_transitions(nfa_model *n, int state, char sym)
{
	/*transition_table_column **col = *(n->ttab);

	int **symbol_transitions = NULL;
	for(int i=0; col[i]; i++)
		if(col[i]->symbol == sym)
		{
			symbol_transitions = col[i]->transitions;
			break;
		}
	*/

	int **symbol_transitions = lookup_trans_col_by_symbol(n, sym);

	if(symbol_transitions == NULL)
		return NULL;
	else
		return symbol_transitions[state];
}

static int **lookup_trans_col_by_symbol(nfa_model *n, char sym)
{
	transition_table_column **col = n->ttab;

	for(int i=0; col[i]; i++)
		if(col[i]->symbol == sym)
		{
			return col[i]->transitions;
		}

	return NULL;
}


