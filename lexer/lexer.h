

#ifndef LEXER_H_
#define LEXER_H_

#include "../symbol.h"
#include "regex.h"

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
