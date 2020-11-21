

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "nfa_build.h"

//
static nfa_model *new_nfa_model(int newstates);
static void import_all_transitions(nfa_model *to, nfa_model *from);
static transition_table_column *create_transition_column(char c, int states);
static void add_transition_to_column(transition_table_column *col, int from, int to);
static void copy_transitions(transition_table_column *to, transition_table_column *from, int state_ct);
static transition_table_column *search_transition_column(transition_table_column **col, char c);

int stindex = 1;

void start_new_nfa(void)
{
	stindex = 1;
}

nfa_model *nfa_create_from_char(char c)
{
	/*nfa_model *n = malloc(sizeof(*n));
	assert(n);

	n->state_ct = stindex+1;	//2 new states
	n->start_state = stindex;

	//allocate transition table
	n->ttab = calloc(129, sizeof(*(n->ttab)));	//129 includes all the chars gottemmmm
	assert(n->ttab);

	//add a column
	n->ttab[0] = create_transition_column(c, n->state_ct + 1);
	add_transition_to_column(n->ttab[0], stindex, stindex+1);

	n->accept_states = malloc(2 * sizeof(*(n->accept_states)));
	assert(n->accept_states);
	n->accept_states[0] = stindex+1;
	n->accept_states[1] = -1;

	stindex += 2;

	return n;*/

	nfa_model *n = new_nfa_model(2);

	//add the column and the transition
	n->ttab[1] = create_transition_column(c, n->state_ct + 1);
	add_transition_to_column(n->ttab[1], n->start_state, n->accept_states[0]);

	return n;
}

nfa_model *nfa_union(nfa_model *n1, nfa_model *n2)
{
	nfa_model *n = new_nfa_model(2);

	//pull transitions from the sub-models
	import_all_transitions(n, n1);
	import_all_transitions(n, n2);


	//add union empty transitions
	add_transition_to_column(n->ttab[0], n->start_state, n1->start_state);
	add_transition_to_column(n->ttab[0], n->start_state, n2->start_state);
	add_transition_to_column(n->ttab[0], n1->accept_states[0], n->accept_states[0]);
	add_transition_to_column(n->ttab[0], n2->accept_states[0], n->accept_states[0]);

	return n;
}

nfa_model *nfa_concatenate(nfa_model *n1, nfa_model *n2)
{
	nfa_model *n = new_nfa_model(2);

	//pull transitions from the sub-models
	import_all_transitions(n, n1);
	import_all_transitions(n, n2);

	//add empty transitions
	add_transition_to_column(n->ttab[0], n->start_state, n1->start_state);
	add_transition_to_column(n->ttab[0], n1->accept_states[0], n2->start_state);
	add_transition_to_column(n->ttab[0], n2->accept_states[0], n->accept_states[0]);

	return n;
}

nfa_model *nfa_kleene(nfa_model *ni)
{
	nfa_model *n = new_nfa_model(2);

	//pull transitions from the sub-model
	import_all_transitions(n, ni);

	//add empty transitions
	add_transition_to_column(n->ttab[0], n->start_state, n->accept_states[0]);
	add_transition_to_column(n->ttab[0], n->start_state, ni->start_state);
	add_transition_to_column(n->ttab[0], ni->accept_states[0], n->accept_states[0]);
	add_transition_to_column(n->ttab[0], ni->accept_states[0], ni->start_state);

	return n;
}

nfa_model *nfa_plus(nfa_model *ni)
{
	nfa_model *n = new_nfa_model(2);

	//pull transitions from the sub-model
	import_all_transitions(n, ni);

	//add empty transitions
	//add_transition_to_column(n->ttab[0], n->start_state, n->accept_states[0]);
	add_transition_to_column(n->ttab[0], n->start_state, ni->start_state);
	add_transition_to_column(n->ttab[0], ni->accept_states[0], n->accept_states[0]);
	add_transition_to_column(n->ttab[0], ni->accept_states[0], ni->start_state);

	return n;
}

nfa_model *nfa_question(nfa_model *ni)
{
	nfa_model *n = new_nfa_model(2);

	//pull transitions from the sub-model
	import_all_transitions(n, ni);

	//add empty transitions
	add_transition_to_column(n->ttab[0], n->start_state, n->accept_states[0]);
	add_transition_to_column(n->ttab[0], n->start_state, ni->start_state);
	add_transition_to_column(n->ttab[0], ni->accept_states[0], n->accept_states[0]);
	//add_transition_to_column(n->ttab[0], ni->accept_states[0], ni->start_state);

	return n;
}

//by default, the first state is the start state, and the last one is the accept state
//ex. if stindex is at 6, and we make a new model w 3 states, then
//start state = 6
//accept state = 8
static nfa_model *new_nfa_model(int newstates)
{
	nfa_model *n = malloc(sizeof(*n));
	assert(n);

	int total_states = stindex + newstates - 1;

	n->state_ct = total_states;
	n->start_state = stindex;

	//allocate transition table
	//n->ttab = calloc(129, sizeof(*(n->ttab)));	//129 includes all the chars gottemmmm
	n->ttab = calloc(256, sizeof(*(n->ttab)));	//129 includes all the chars gottemmmm
	assert(n->ttab);

	//add a column
	n->ttab[0] = create_transition_column(EMPTY_SYMBOL, total_states+1);

	//this is gonna get replaced with just an int for the (Single) accept state
	n->accept_states = malloc(2 * sizeof(*(n->accept_states)));
	assert(n->accept_states);
	n->accept_states[0] = total_states;
	n->accept_states[1] = -1;

	stindex += newstates;

	return n;
}

static void import_all_transitions(nfa_model *to, nfa_model *from)
{
	//find the last column in the destination model (the point at which we start copying)
	transition_table_column **dcol = to->ttab;
	int dcol_avail;
	for(dcol_avail=0; dcol[dcol_avail]; dcol_avail++);

	//add transitions columns from "from"
	transition_table_column **col = from->ttab;
	for(int i=0; col[i]; i++)
	{
		if(col[i]->symbol != EMPTY_SYMBOL)
		{
			transition_table_column *d_target = search_transition_column(dcol, col[i]->symbol);
			if(d_target == NULL)
			{
				//d_target = dcol[dcol_avail];	//or to->ttab[dcol_avail]?
				dcol[dcol_avail] = create_transition_column(col[i]->symbol, to->state_ct + 1);
				copy_transitions(dcol[dcol_avail], col[i], from->state_ct);
				dcol_avail++;
			}
			else
				copy_transitions(d_target, col[i], from->state_ct);
		}
		else	//this might be wrong -- untested
			copy_transitions(to->ttab[0], col[i], from->state_ct);
	}
}

static transition_table_column *create_transition_column(char c, int states)
{
	transition_table_column *col = malloc(sizeof(*col));
	assert(col);

	col->symbol = c;
	col->transitions = calloc(states, sizeof(int*));
	assert(col->transitions);

	return col;
}

static void add_transition_to_column(transition_table_column *col, int from, int to)
{
	int **t = &(col->transitions[from]);

	if(*t == NULL)
	{
		*t = malloc(2 * sizeof(**t));
		(*t)[0] = to;
		(*t)[1] = -1;
	}
	else
	{
		int t_already;
		for(t_already=0; (*t)[t_already] != -1; t_already++);
		*t = realloc(*t, (t_already+2) * sizeof(int));
		//assert
		(*t)[t_already] = to;
		(*t)[t_already+1] = -1;
	}
}

//doesn't overwrite, only adds
//!!!!!!! this needs to check if the transition already exists!! (or does it)
static void copy_transitions(transition_table_column *to, transition_table_column *from, int state_ct)
{
	for(int i=0; i<state_ct; i++)
	{
		int *tr = (from->transitions)[i];
		if(tr)
		{
			//count transitions from that state
			int j;
			for(j=0; tr[j] != -1; j++);

			//allocate
			size_t tr_mem = (j+1) * sizeof(int);
			(to->transitions)[i] = malloc(tr_mem);

			//copy
			memcpy((to->transitions)[i], tr, tr_mem);
		}
	}
}

static transition_table_column *search_transition_column(transition_table_column **col, char c)
{
	for(int i=0; col[i]; i++)
	{
		if(col[i]->symbol == c)
			return col[i];
	}

	return NULL;
}

void nfa_dump(nfa_model *n)
{
	printf("dumping nfa...\n");
	printf("start state:\t%d\n", n->start_state);

	printf("accept states:\t");
	for(int *a = n->accept_states; *a != -1; a++)
		printf("%d ", *a);

	transition_table_column **col = n->ttab;
	
	for(int i=0; col[i]; i++)
	{
		//printf("\ntransitions for \'%c\':\t", ((unsigned char)(col[i]->symbol)==EMPTY_SYMBOL)? 137 : col[i]->symbol);
		printf("\ntransitions for \'%c\':\t", ((col[i]->symbol)==EMPTY_SYMBOL)? 137 : col[i]->symbol);
		for(int s=0; s<n->state_ct; s++)
		{
			if(col[i]->transitions[s])
			{
				printf("%d->", s);
				if((col[i]->transitions[s])[1] == -1)
				{
					printf("%d", (col[i]->transitions[s])[0]);
				}
				else
				{
					printf("(");
					for(int tr=0; (col[i]->transitions[s])[tr] != -1; tr++)
						printf("%d ", (col[i]->transitions[s])[tr]);
					printf(")");
				}
				printf(" ");
				//for()
			}
		}
	}

	putchar('\n');
}

