

#ifndef REGEX_H_
#define REGEX_H_

#include "../../swglib/automata/nfa/nfa_build.h"
#include "../../swglib/automata/nfa/nfa_run.h"

typedef struct node_s node;
typedef enum tok_type_e tok_type;

nfa_model *regex_compile(const char *regex);


//void print_ptree(node *pt);
//char *ptree_compose_string(node *n);

void ptree_extract_dfs(node *n);

void nfa_builder_initialize(void);
nfa_model *ptree_to_nfa(node *n);

#endif //REGEX_H_
