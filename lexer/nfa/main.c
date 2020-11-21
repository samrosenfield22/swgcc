/**/

#include "nfa_build.h"
#include "nfa_run.h"


/*
transition_table_column e_trans = {EMPTY_SYMBOL, {(int[]){1,7,-1}, (int[]){2,4,-1}, NULL, (int[]){6,-1}, NULL, (int[]){6,-1}, (int[]){1,7,-1}, NULL, NULL, NULL, NULL}};
transition_table_column a_trans = {'a', {NULL, NULL, (int[]){3,-1}, NULL, NULL, NULL, NULL, (int[]){8,-1}, (int[]){9,-1}, NULL, NULL}};
transition_table_column b_trans = {'b', {NULL, NULL, NULL, NULL, (int[]){5,-1}, NULL, NULL, NULL, NULL, (int[]){10,-1}, NULL}};
transition_table_column *aorb_aab_trans[] = {&e_trans, &a_trans, &b_trans, NULL};

//[0-9]+					decimal
//0x[A-Fa-z0-9]+			hex
//0b(0|1)+					bin
//[A-Za-z]+[A-Za-z0-9]*		identifier


int accept_states[] = {10, -1};

nfa_model test_nfa =
{
	&aorb_aab_trans,
	accept_states,
	0
};

*/

int main(void)
{
	nfa_simulator_initialize();
	
	char *strings[] = {"", "a", "b", "ab", "ba", "aa", "bb", "aab", "aba", "aaa", "bba",
	"aaab", "abaab", "bbbbaab", "aabaaba", "baba", "abaa", "aaaaaaab", "bbaabb", "abc"};
	/*for(int i=0; i<sizeof(strings)/sizeof(strings[0]); i++)
	{
		//printf("%s\n", strings[i]);
		if(nfa_run(&test_nfa, strings[i]))
			puts(strings[i]);
	}*/

	/*nfa_model *only_a = nfa_create_from_char('a');
	nfa_model *only_b = nfa_create_from_char('b');

	//nfa_model *a_union_b = nfa_union(only_a, only_b);
	//nfa_model *a_concat_b = nfa_concatenate(only_a, only_b);
	nfa_model *a_kleene = nfa_kleene(only_a);

	nfa_dump(a_kleene);*/
	//nfa_dump(a_kleene);

	//(a|b)+aab
	nfa_model *m1 = nfa_union(nfa_create_from_char('a'), nfa_create_from_char('b'));
	nfa_model *m2 = nfa_kleene(m1);
	nfa_model *a1 = nfa_create_from_char('a');
	nfa_model *m3 = nfa_concatenate(m2, a1);
	nfa_model *m4 = nfa_concatenate(m3, nfa_create_from_char('a'));
	nfa_model *m5 = nfa_concatenate(m4, nfa_create_from_char('b'));

	//return 0;
	/*nfa_dump(m1);
	nfa_dump(m2);
	nfa_dump(m3);*/

	for(int i=0; i<sizeof(strings)/sizeof(strings[0]); i++)
	{
		int moves = nfa_run(m5, strings[i]);
		if(moves != -1)
			printf("%s\t\t%d moves\n", strings[i], moves);
	}

	return 0;
}