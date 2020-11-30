

#ifndef RECDESC_H_
#define RECDESC_H_

#include "recdesc_types.h"
#include "grammar.h"
#include "ptree.h"


//
lextok *chars_to_substrings_lexer(const char *instr);

pnode *parse(lextok *lex_tokens_in, bool verbose);
pnode *parse_nonterm(nonterminal_type nt);

bool consume_nonterm(pnode *root, prod_tok *tok, prod_tok *next_tok);
bool consume_term_or_ident(pnode *root, prod_tok *tok);

int parse_table_lookup(nonterminal_type nt);
bool match(prod_tok *tok);
void next(void);

#endif //RECDESC_H_
