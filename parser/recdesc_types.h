

#ifndef RECDESC_TYPES_H
#define RECDESC_TYPES_H

//these will actually be some struct *
typedef struct lextok_s
{
	char *str;
	bool is_ident;
	int ident_id;
} lextok;

//eventually will get auto-gened/non needed
/*typedef enum nonterminal_type_e
{
	REGEX,
	MORETERM,
	TERM,
	FACTOR,
	CONFACTOR,
	BASE_SUFFIX,
	BASE,
	RANGE
} nonterminal_type;*/
typedef int nonterminal_type;

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
		//nonterminal_type nonterm;
		int nonterm;
		/*char *term;
		char *semact;
		char expr;
		char *ident;*/
		char *str;
	};
} prod_tok;

typedef struct production_rule_s
{
	nonterminal_type lhs;
	prod_tok **rhs;
} production_rule;

typedef struct grammar_s
{
    production_rule *rules;
    char **nonterminals, **terminals;
    int alphabet_len, nonterm_len, grammar_len;
		int *parse_table;
} grammar;

#endif //RECDESC_TYPES_H
