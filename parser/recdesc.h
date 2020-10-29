

#ifndef RECDESC_H_
#define RECDESC_H_

#include "recdesc_types.h"
#include "tree.h"




//
lextok *chars_to_substrings_lexer(const char *instr);

void productions_to_parse_table(void);
void mark_entries_for_nonterminal(nonterminal_type nt);

node *parse(lextok *lex_tokens_in);
node *parse_nonterm(nonterminal_type nt);
int parse_table_lookup(nonterminal_type nt);
int find_parse_table_column(const char *symbol);
bool match(prod_tok *tok);
void next(void);

#endif //RECDESC_H_