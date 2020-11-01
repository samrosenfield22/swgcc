

/*
	0	regex -> term | regex
	1			| term

	2	term -> factor+

	3	factor -> base*
	4			| base+
	5			| base?
				| base

	6	base -> char
	7			| (regex)
	8			| [range+]

	9	range -> char-char

	---------------------------------

	rewritten, given the following rules:
	* no left-recursion
	* for each nonterminal, either
		* it must have one production, or
		* it can have multiple productions, each of which start with distinct terminals
		ex.
		factor -> base* | base+ | base?
		becomes
		factor -> base suffix?
		suffix -> * | + | ?
	* if a token is followed by an expr (ie "factor+" or "thing?"), this must either be the end of the production,
		or the following token must not be a nonterminal

	0	regex -> term moreterm*
	1	moreterm -> | term
	2	term -> factor+
	3	factor -> base base_suffix?
	4	base_suffix -> *
	5					| +
	6					| ?
	7	base -> char
	8			| (regex)
	9			| [range+]
	a	range -> char-char

	---------------------------------

	grammar to parse table:
		if the first item on the rhs is a nonterminal, we always apply that production (fill the whole row)
		if the first item on the rhs is a terminal, write to the one cell table[nt][term] = production_id

				c 	* 	+ 	? 	( 	)	[	]	|
	regex 		0	0	0	0	0	0	0	0	0
	term 		2	2	2	2	2	2	2	2	2
	moreterm 	-	-	-	-	-	-	-	-	1
	factor 		3	3	3	3	3	3	3	3	3
	base 		7	-	-	-	8	-	9	-	-
	bsuffix		-	4	5	6	-	-	-	-	-
	range		a	-	-	-	-	-	-	-	-

	---------------------------------

	each item on the rhs of a production is one of the following:
	terminal (reserved), id, nonterminal, semantic action, expr (*,+,?)


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "recdesc.h"

#define P_OK true
#define P_FAIL false
bool PARSER_STATUS = P_OK;


lextok *lex_tokens;
lextok *lex_tok;

//the lhs of the first production is the start symbol
#define grammar_start_symbol (0)

//this gets created/populated by the lexer
//maybe a table that matches a regex to its associated ident, i.e.
/*
struct
{
	char *regex;
	nfa *model;
	lextok default_tok;
}
{"* | + | ? | ...", 			NULL, (lextok){NULL, false, 0}};
{"[A-Za-z][A-Za-z0-9]*", 	NULL, (lextok){NULL, true, 0}};		//id
{"[0-9]+", 								NULL, (lextok){NULL, true, 1}};		//num
{"0x[A-Fa-f0-9]+", 				NULL, (lextok){NULL, true, 2}};		//hex
//where the 0,1,2 (the ident_id values) are indices to a table char* [] = {"id", "num", "hex", ...};
*/
lextok *chars_to_substrings_lexer(const char *instr)
{
		lextok *l = calloc(strlen(instr)+1, sizeof(*l));
		assert(l);
		lextok *lp = l;

		const char *ops = "*+?|()[]-";
		for(const char *c=instr; *c!='\0'; c++)
		{
			lp->str = malloc(2);
			lp->str[0] = *c;
			lp->str[1] = '\0';
			if(strchr(ops, *c))
			{
					lp->is_ident = false;
			}
			else if((*c>='a' && *c<='z') || (*c>='A' && *c<='Z'))
			{
					lp->is_ident = true;
					lp->ident_id = 0;
			}
			else if(*c>='0' && *c<='9')
			{
					lp->is_ident = true;
					lp->ident_id = 1;
			}
			else assert(0);

			lp++;
		}

		lp->str = NULL;
		return l;
}


node *parse(lextok *lex_tokens_in)
{
	lex_tokens = lex_tokens_in;
	lex_tok = lex_tokens;

	node *tree = parse_nonterm(grammar_start_symbol);

	if(lex_tok->str != NULL)
		return false;
	else
		return tree;
}

#define PARSER_FAILURE do {PARSER_STATUS = P_FAIL; return NULL;} while(0)
//#define BAIL_IF_PARSER_FAILED if(PARSER_STATUS != P_OK) return NULL
#define BAIL_IF_PARSER_FAILED if(PARSER_STATUS != P_OK) {ptree_traverse_dfs(root, NULL, node_delete, false); return NULL;}

node *parse_nonterm(nonterminal_type nt)
{
	//printf("\nparsing nonterminal: %s\n", loaded_grammar->nonterminals[nt]);
	//printf("\tlookahead at lex tok %d (%s)\n", lex_tok-lex_tokens, lex_tok->str);

	node *root = node_create(true, nt, NULL, NULL);
	int ci = 0;

	//look in the parse table, get the next production to apply
	int next_production = parse_table_lookup(nt);
	//printf("\tapplying production %d\n", next_production);
	//if(next_production == -1)
	//	PARSER_FAILURE;

	for(prod_tok **rhs = gg.rules[next_production].rhs; *rhs; rhs++)
	{
		prod_tok *tok = *rhs, *next_tok = *(rhs+1);
		switch(tok->type)
		{
			case NONTERMINAL:
				if(!consume_nonterm(root, &ci, tok, next_tok))
					BAIL_IF_PARSER_FAILED
				break;

			case TERMINAL:
			case IDENT:
				if(!consume_term_or_ident(root, &ci, tok))
					PARSER_FAILURE;
				break;

			case SEMACT:	root->children[ci++] = node_create(false, tok->type, tok->str, NULL); break;
			case EXPR: 		break;
		}
	}

	//printf("done parsing %s\n\n", loaded_grammar->nonterminals[nt]);
	return root;
}

bool consume_nonterm(node *root, int *ci, prod_tok *tok, prod_tok *next_tok)
{
	//printf("consuming nonterm %s\n", loaded_grammar->nonterminals[tok->nonterm]);
	//printf("\tlookahead at lex tok %d (%s)\n", lex_tok-lex_tokens, lex_tok->str);

	bool always_do_first = true, repeat = false;

	if(next_tok && next_tok->type == EXPR)
	{
		char e_char = *(next_tok->str);
		assert(e_char=='*' || e_char=='+' || e_char=='?');

		always_do_first = (e_char == '+');
		repeat = (e_char=='*' || e_char=='+');
	}

	if(always_do_first)
		goto skip_first_check;

	do {
		if(parse_table_lookup(tok->nonterm) == -1)
			return true;

		skip_first_check:
		root->children[(*ci)++] = parse_nonterm(tok->nonterm);
		if(PARSER_STATUS != P_OK)
			return false;
	} while(repeat);

	return true;
}

bool consume_term_or_ident(node *root, int *ci, prod_tok *tok)
{
	//printf("\t\tconsuming term/ident %s\n", tok->str);
	if(!match(tok))
	{
		//printf("failed to match token \'%s\' (type %s)\n", tok->str, t_strings[tok->type]);
		return false;
	}

	symbol *sym = (tok->type==IDENT)? lex_tok->sym : NULL; 
	root->children[(*ci)++] = node_create(false, tok->type, lex_tok->str, sym);
	//if(lex_tok->is_ident)
	//	root->children[(*ci)++] = node_create(false, SEMACT, lex_tok->str);	//just for testing
	next();
	return true;
}

int parse_table_lookup(nonterminal_type nt)
{
	if(!lex_tok->str)
		return -1;

	//int col = find_parse_table_column(lex_tok->str);
	int col;
	if(lex_tok->is_ident)
	{
			col = find_parse_table_column(ident_table[lex_tok->ident_id], IDENT);
	}
	else
	{
			col = find_parse_table_column(lex_tok->str, TERMINAL);
	}
	//int col = find_parse_table_column(lex_tok->ident_id, );
	if(col == -1)
		col = 0;

	//return parse_table[table_entry(nt, col)];
	int index = nt*gg.alphabet_len + col;
	return gg.parse_table[index];
}

//the token must either be a terminal, or NULL (end of tokens)
bool match(prod_tok *tok)
{
	if(lex_tok->str == NULL)
		return false;
	else if(!tok)
		return false;
		//return (lex_tok->str == NULL);
	else if(tok->type == TERMINAL)
	{
		//printf("lex tok is a %s\n", lex_tok->is_ident)
		if(lex_tok->is_ident) return false;
		else return (strcmp(tok->str, lex_tok->str) == 0);
	}
	else if(tok->type == IDENT)
		return lex_tok->is_ident;
	else
		return false;
}

void next(void)
{
	lex_tok++;
}
