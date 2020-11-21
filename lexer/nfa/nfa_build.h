

#ifndef NFA_BUILD_H_
#define NFA_BUILD_H_

#include "nfa_run.h"

void start_new_nfa(void);
nfa_model *nfa_create_from_char(char c);
nfa_model *nfa_union(nfa_model *n1, nfa_model *n2);
nfa_model *nfa_concatenate(nfa_model *n1, nfa_model *n2);
nfa_model *nfa_kleene(nfa_model *n);
nfa_model *nfa_plus(nfa_model *n);
nfa_model *nfa_question(nfa_model *n);

void nfa_dump(nfa_model *n);

#endif //NFA_H_