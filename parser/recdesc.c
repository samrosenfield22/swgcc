

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

nonterminal_type grammar_start_symbol = REGEX;



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
			lp->is_ident = (strchr(ops, *c)==NULL);

			lp++;
		}

		lp->str = NULL;
		return l;
}


/*
	0	regex -> term moreterm*
	1	moreterm -> | term
	2	term -> factor confactor*
	3	factor -> base base_suffix?
	4	base_suffix -> *
	5					| +
	6					| ?
	7	base -> char
	8			| (regex)
	9			| [range+]
	a	range -> char-char
	b 	confactor -> factor {concat}
*/


//the rhs of a production is an array of type prod_tok
//generate this from the BNF spec or whatever
#define NT_TOK(i)	&(prod_tok){NONTERMINAL, .nonterm=i}
#define T_TOK(i)	&(prod_tok){TERMINAL, .term=i}
#define SEM_TOK(i)	&(prod_tok){SEMACT, .semact=i}
#define EXPR_TOK(i)	&(prod_tok){EXPR, .expr=i}
#define IDENT_TOK()	&(prod_tok){IDENT, .ident=NULL}

production_rule productions[] =
{
	[0] =	{REGEX, (prod_tok*[]){NT_TOK(TERM), NT_TOK(MORETERM), EXPR_TOK('*'), NULL}},
	[1] =	{MORETERM, (prod_tok*[]){T_TOK("|"), NT_TOK(TERM), SEM_TOK("union"), NULL}},
	[2] =	{TERM, (prod_tok*[]){NT_TOK(FACTOR), NT_TOK(CONFACTOR), EXPR_TOK('*'), NULL}},		//the concat sem_tok needs to be inside the repetition!
	[3] =	{FACTOR, (prod_tok*[]){NT_TOK(BASE), NT_TOK(BASE_SUFFIX), EXPR_TOK('?'), NULL}},
	[4] =	{BASE_SUFFIX, (prod_tok*[]){T_TOK("*"), SEM_TOK("0 or more"), NULL}},
	[5] =	{BASE_SUFFIX, (prod_tok*[]){T_TOK("+"), SEM_TOK("1 or more"), NULL}},
	[6] =	{BASE_SUFFIX, (prod_tok*[]){T_TOK("?"), SEM_TOK("0 or 1"), NULL}},
	[7] =	{BASE, (prod_tok*[]){IDENT_TOK(), SEM_TOK("push"), NULL}},
	[8] =	{BASE, (prod_tok*[]){T_TOK("("), NT_TOK(REGEX), T_TOK(")"), NULL}},
	[9] =	{BASE, (prod_tok*[]){T_TOK("["), NT_TOK(RANGE), T_TOK("]"),  NULL}},
	[10] =	{RANGE, (prod_tok*[]){IDENT_TOK(), SEM_TOK("push"), T_TOK("-"), IDENT_TOK(), SEM_TOK("push"), SEM_TOK("range"), NULL}},
	[11] =	{CONFACTOR, (prod_tok*[]){NT_TOK(FACTOR), SEM_TOK("concat"), NULL}}
};

//parse table should be generated programatically from the productions
//the 0th column is for idents -- arbitrary strings (or chars)
/*
				s 	* 	+ 	? 	( 	)	[	]	|
	regex 		0	0	0	0	0	0	0	0	0
	term 		2	2	2	2	2	2	2	2	2
	moreterm 	-	-	-	-	-	-	-	-	1
	factor 		3	3	3	3	3	3	3	3	3
	base 		7	-	-	-	8	-	9	-	-
	bsuffix		-	4	5	6	-	-	-	-	-
	range		a	-	-	-	-	-	-	-	-

*/
//-1 is a "magic number" -- it means an invalid option
const char *parse_table_columns[] = {NULL, "*", "+", "?", "(", ")", "[", "]", "|"};
/*int manual_parse_table[8][9] =
{
	[REGEX] =		{0, 0, 0, 0, 0, 0, 0, 0, 0},
	[MORETERM] =	{-1, -1, -1, -1, -1, -1, -1, -1, 1},
	//[TERM] =		{2, 2, 2, 2, 2, 2, 2, 2, 2},
	[TERM] =		{2, -1, -1, -1, 2, -1, 2, -1, -1},
	//[FACTOR] =		{3, 3, 3, 3, 3, 3, 3, 3, 3},
	[FACTOR] =		{3, -1, -1, -1, 3, -1, 3, -1, -1},
	[CONFACTOR] =	{11, -1, -1, -1, 11, -1, 11, -1, -1},
	[BASE_SUFFIX] =	{-1, 4, 5, 6, -1, -1, -1, -1, -1},
	[BASE] =		{7, -1, -1, -1, 8, -1, 9, -1, -1},
	[RANGE] =		{10, -1, -1, -1, -1, -1, -1, -1, -1}
};*/

int *parse_table;


//int *parse_table = (int*)manual_parse_table;
int alphabet_len = 8;	//not including idents
int nt_cnt = 8;
int production_cnt = sizeof(productions) / sizeof(productions[0]);

bool *production_marked;
#define table_entry(nt, alphabet_index) (nt*(alphabet_len+1) + alphabet_index)
void productions_to_parse_table(void)
{
	int table_entries = (alphabet_len+1) * nt_cnt;
	parse_table = malloc(table_entries * sizeof(*parse_table));
	assert(parse_table);

	for(int i=0; i<table_entries; i++)
		parse_table[i] = -1;

	production_marked = malloc(production_cnt * sizeof(*production_marked));
	assert(production_marked);
	for(int i=0; i<production_cnt; i++)
		production_marked[i] = false;

	//mark_entries_for_nonterminal(grammar_start_symbol);
	//mark_entries_for_nonterminal(BASE_SUFFIX);
	for(int i=0; i<production_cnt; i++)
		mark_entries_for_nonterminal(i);

}

void mark_entries_for_nonterminal(nonterminal_type nt)
{
	if(production_marked[nt] == true)
		return;

	printf("marking entries for nonterminal: %s\n", nt_strings[nt]);
	/*
	for each production for that nonterminal
		if the first (non-semantic) element of the rhs is a terminal/ident
			ptab[table_entry(nt, term)] = that production's id
		else if it's a nonterminal
			make sure that's the ONLY production for that nonterminal (else error)
			recurse on that nonterminal
	*/

	int alpha_col;

	//iterate through all productions for this nonterminal
	for(int i=0; i<production_cnt; i++)
	{
		if(productions[i].lhs != nt)
			continue;

		//grab the first token of the production's rhs
		prod_tok **rhs = productions[i].rhs;
		prod_tok *firsttok = *rhs;
		switch(firsttok->type)
		{
			case SEMACT: case EXPR: assert(0); /*break;*/

			case TERMINAL:
			case IDENT:
				if(firsttok->type == IDENT)
					alpha_col = 0;
				else
				{
					alpha_col = find_parse_table_column(firsttok->term);
					assert(alpha_col != -1);
				}

				parse_table[table_entry(nt, alpha_col)] = i;
				break;

			case NONTERMINAL:

			//make sure that's the ONLY production for that nonterminal
			//for now we're just assuming that's the case...
			for(int j=i+1; j<production_cnt; j++)
				if(productions[j].lhs == nt)
					assert(0);

			//recurse on that nonterminal
			mark_entries_for_nonterminal(firsttok->nonterm);

			//copy all the ones that aren't -1
			for(int j=0; j<alphabet_len; j++)
			{
				if(parse_table[table_entry(firsttok->nonterm, j)] != -1)
					parse_table[table_entry(nt, j)] = i;
			}
		}
	}

	//mark this production as complete
	production_marked[nt] = true;
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

//import_productions()
//generate_parse_table()

#define PARSER_FAILURE do {PARSER_STATUS = P_FAIL; return NULL;} while(0)
//#define PARSER_FAILURE do {PARSER_STATUS = FAIL; return NULL;} while(0)
//#define BAIL_IF_PARSER_FAILED if(PARSER_STATUS != P_OK) return NULL
#define BAIL_IF_PARSER_FAILED if(PARSER_STATUS != P_OK) {ptree_traverse_dfs(root, node_delete, false); return NULL;}

node *parse_nonterm(nonterminal_type nt)
{
	printf("\nparsing nonterminal: %s\n", nt_strings[nt]);
	printf("\tlookahead at lex tok %d (%s)\n", lex_tok-lex_tokens, lex_tok->str);

	node *root = node_create(true, nt, NULL);
	int ci = 0;

	//look in the parse table, get the next production to apply
	int next_production = parse_table_lookup(nt);
	printf("\tapplying production %d\n", next_production);
	//if(next_production == -1)
	//	PARSER_FAILURE;

	for(prod_tok **rhs = productions[next_production].rhs; *rhs; rhs++)
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

			case SEMACT:	root->children[ci++] = node_create(false, tok->type, tok->semact); break;
			case EXPR: 		break;
		}
	}

	return root;
}

bool consume_nonterm(node *root, int *ci, prod_tok *tok, prod_tok *next_tok)
{
	bool always_do_first = true, repeat = false;

	if(next_tok && next_tok->type == EXPR)
	{
		always_do_first = (next_tok->expr == '+');
		repeat = (next_tok->expr == '*' || next_tok->expr == '+');
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
	if(!match(tok))
	{
		printf("failed to match token \'%s\' (type %s)\n", tok->term, t_strings[tok->type]);
		return false;
	}
	
	root->children[(*ci)++] = node_create(false, tok->type, lex_tok->str);
	if(lex_tok->is_ident)
		root->children[(*ci)++] = node_create(false, SEMACT, lex_tok->str);	//just for testing
	next();
	return true;
}

int parse_table_lookup(nonterminal_type nt)
{
	if(!lex_tok->str)
		return -1;

	//lookup the column in the parse table
	/*bool col_found = false;
	int col;
	for(col=1; col < sizeof(parse_table_columns)/sizeof(parse_table_columns[0]); col++)
	{
		if(strcmp(parse_table_columns[col], lex_tok->str) == 0)
		{
			col_found = true;
			break;
		}
	}
	if(!col_found)
		col = 0;*/

	int col = find_parse_table_column(lex_tok->str);
	if(col == -1)
		col = 0;

	//return parse_table[nt][col];

	int num_col = sizeof(parse_table_columns)/sizeof(parse_table_columns[0]);
	return parse_table[nt*num_col + col];
}

int find_parse_table_column(const char *symbol)
{
	for(int col=1; col < sizeof(parse_table_columns)/sizeof(parse_table_columns[0]); col++)
	{
		if(strcmp(parse_table_columns[col], symbol) == 0)
		{
			return col;
		}
	}

	return -1;	//not found
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
		else return (strcmp(tok->term, lex_tok->str) == 0);
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
