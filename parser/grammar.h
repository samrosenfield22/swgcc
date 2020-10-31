

#ifndef GRAMMAR_H_
#define GRAMMAR_H_

#include "recdesc_types.h"

extern grammar gg;
extern char *ident_table[];

grammar *load_grammar(const char *fname);

//int find_parse_table_column(const char *symbol);
int find_parse_table_column(char *str, prod_tok_type type);
void dump_parse_table(int *pt);

void dump_classnames(void);
void dump_productions(grammar *g);

#endif //GRAMMAR_H_
