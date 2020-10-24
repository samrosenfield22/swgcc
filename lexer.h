

#ifndef LEXER_H_
#define LEXER_H_

#include "symbol.h"
#include "regex/regex.h"
//#include "../swglib/automata/nfa/nfa.h"

typedef enum lex_tok_type_e
{
    LITERAL,
    IDENTIFIER,
    OPERATOR
} lex_tok_type;

typedef struct lex_token_s
{
    lex_tok_type type;
    int litval;
    char *opstr;
    symbol *sym;
} lex_token;

void lexer_build_all_regexes(void);
lex_token *lexer(const char *str);

lex_token *lexer(const char *str);

#endif //LEXER_H_
