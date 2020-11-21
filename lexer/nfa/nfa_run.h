/**/

#ifndef NFA_RUN_H_
#define NFA_RUN_H_

//#include "../../../swglib/structures/stack/stack.h"
#include "stack/stack.h"

#include <stdbool.h>

#define EMPTY_SYMBOL (-128)

typedef struct transition_table_column_s
{
	char symbol;
	//int *transitions[];
	int **transitions;
} transition_table_column;

//typedef transition_table_column *transition_table;

typedef struct nfa_model_s
{
	//transition_table *ttab;
	transition_table_column **ttab;
	int *accept_states;
	int start_state;
	int state_ct;
} nfa_model;


void nfa_simulator_initialize(void);

int nfa_run(nfa_model *n, const char *s);
void nfa_transition(nfa_model *n, int state, char c, int sindex);
bool nfa_is_accept_state(nfa_model *n, int state);


#endif //NFA_RUN_H_