

#ifndef RECDESC_TYPES_H
#define RECDESC_TYPES_H

//these will actually be some struct *
typedef struct lextok_s
{
	char *str;
	bool is_ident;	//type
} lextok;

//eventually will get auto-gened/non needed
typedef enum nonterminal_type_e
{
	REGEX,
	MORETERM,
	TERM,
	FACTOR,
	CONFACTOR,
	BASE_SUFFIX,
	BASE,
	RANGE
} nonterminal_type;

//types of tokens in the rhs of a production rule
typedef enum prod_tok_type_e
{
	NONTERMINAL,
	IDENT,
	TERMINAL,
	SEMACT,
	EXPR
} prod_tok_type;

//production rule tokens
typedef struct prod_tok_s
{
	prod_tok_type type;
	union
	{
		nonterminal_type nonterm;
		char *term;
		char *semact;
		char expr;
		char *ident;
	};
} prod_tok;

typedef struct production_rule_s
{
	nonterminal_type lhs;
	prod_tok **rhs;
} production_rule;

#endif //RECDESC_TYPES_H