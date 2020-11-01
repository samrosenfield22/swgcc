

#ifndef LEXER_H_
#define LEXER_H_

#include "../symbol.h"
#include "regex.h"
//#include "../swglib/automata/nfa/nfa.h"

/*typedef enum lex_tok_type_e
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
} lex_token;*/

typedef struct lextok_s
{
	char *str;
	bool is_ident;
	int ident_id;

	int val;	//for numerical literal tokens
	symbol *sym;
} lextok;

void lexer_initialize(void);
lextok *lexer(const char *str);
void lex_tokens_dump(lextok *lt);

extern char *ident_table[];
extern int ident_len;


#endif //LEXER_H_
