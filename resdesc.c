

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

#define P_OK true
#define P_FAIL false
bool PARSER_STATUS = P_OK;

//these will actually be some struct *
typedef struct lextok_s 
{
	char *str;
	bool is_ident;	//type
} lextok;
lextok *lex_tokens;
lextok *lex_tok;

//eventually will get auto-gened/non needed
typedef enum nonterminal_type_e
{
	REGEX,
	MORETERM,
	TERM,
	FACTOR,
	BASE_SUFFIX,
	BASE,
	RANGE
} nonterminal_type;

const char *nt_strings[] =
{
	[REGEX] = "regex",
	[MORETERM] = "moreterm",
	[TERM] = "term",
	[FACTOR] = "factor",
	[BASE_SUFFIX] = "base_suffix",
	[BASE] = "base",
	[RANGE] = "range",
};

nonterminal_type grammar_start_symbol = REGEX;

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
//can i put the enum in here?? will it still be available outside this? that'd be amazing if so
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

//parse tree node
typedef struct node_s node;
struct node_s
{
    nonterminal_type type;
    node *children[20];
    char *str;
};

//
node *parse(lextok *lex_tokens_in);
node *parse_nonterm(nonterminal_type nt);
int parse_table_lookup(nonterminal_type nt);
bool match(prod_tok *tok);
void next(void);
node *node_create(nonterminal_type type, const char *str);
void ptree_print(node *pt);
void ptree_print_recursive(node *pt, int depth);

int main(void)
{
	lextok dummy[3];
	dummy[0].str = "a";
	dummy[0].is_ident = true;
	dummy[1].str = "|";
	dummy[1].is_ident = false;
	dummy[2].str = "b";
	dummy[3].is_ident = true;
	

	void *tree = parse(dummy);
	if(!tree) {printf("\nparser failed!\n"); return 0;}
	ptree_print(tree);
	return 0;
}

/*
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
*/

//the rhs of a production is an array of type prod_tok
//generate this from the BNF spec or whatever
#define NT_TOK(i)	&(prod_tok){NONTERMINAL, .nonterm=i}
#define T_TOK(i)	&(prod_tok){TERMINAL, .term=i}
#define SEM_TOK(i)	&(prod_tok){SEMACT, .semact=i}
#define EXPR_TOK(i)	&(prod_tok){EXPR, .expr=i}
#define IDENT_TOK()	&(prod_tok){IDENT, .ident=NULL}
//#define NULL_TOK	(prod_tok)(NULL)
/*prod_tok *productions[] =
{
	[0]	=	&(prod_tok[]){NT_TOK(TERM), NT_TOK(MORETERM), EXPR_TOK("*"), NULL_TOK},		//regex -> term moreterm*
	[1] =	&(prod_tok[]){T_TOK("|"), NT_TOK(TERM), NULL_TOK},			//moreterm -> | term
	[2] =	&(prod_tok[]){NT_TOK(FACTOR), EXPR_TOK("+"), NULL_TOK},			//term -> factor+
	[3] =	&(prod_tok[]){NT_TOK(BASE), NT_TOK(BASE_SUFFIX), EXPR_TOK("?"), NULL_TOK},		//factor -> base base_suffix?
	[4] =	&(prod_tok[]){T_TOK("*"), NULL_TOK},			//base_suffix -> * 
	[5] =	&(prod_tok[]){T_TOK("+"), NULL_TOK},			// | +
	[6] =	&(prod_tok[]){T_TOK("?"), NULL_TOK},			// | ?
	[7] =	&(prod_tok[]){IDENT_TOK(), NULL_TOK},			//base -> char 
	[8] =	&(prod_tok[]){T_TOK("("), NT_TOK(REGEX), T_TOK(")"), NULL_TOK},			// | (regex)
	[9] =	&(prod_tok[]){T_TOK("["), NT_TOK(RANGE), T_TOK("]"),  NULL_TOK},			// | [range}]
	[10] =	&(prod_tok[]){IDENT_TOK(), T_TOK("-"), IDENT_TOK(), NULL_TOK}				//range -> char-char
};*/

//prod_tok *dummy_production[] = {NT_TOK(TERM), NT_TOK(MORETERM), EXPR_TOK('*'), NULL};
//prod_tok **dummy_production = (*prod_tok[]){NT_TOK(TERM), NT_TOK(MORETERM), EXPR_TOK('*'), NULL};

//a token is a *prod_tok
//a production is **prod_tok
//the grammar is **prod_tok[]
prod_tok **productions[] = 
{
	//[0] = dummy_production
	[0] =	(prod_tok*[]){NT_TOK(TERM), NT_TOK(MORETERM), EXPR_TOK('*'), NULL},
	[1] =	(prod_tok*[]){T_TOK("|"), NT_TOK(TERM), NULL},
	[2] =	(prod_tok*[]){NT_TOK(FACTOR), EXPR_TOK('+'), NULL},
	[3] =	(prod_tok*[]){NT_TOK(BASE), NT_TOK(BASE_SUFFIX), EXPR_TOK('?'), NULL},
	[4] =	(prod_tok*[]){T_TOK("*"), NULL},
	[5] =	(prod_tok*[]){T_TOK("+"), NULL},
	[6] =	(prod_tok*[]){T_TOK("?"), NULL},
	[7] =	(prod_tok*[]){IDENT_TOK(), SEM_TOK(""), NULL},
	[8] =	(prod_tok*[]){T_TOK("("), NT_TOK(REGEX), T_TOK(")"), NULL},
	[9] =	(prod_tok*[]){T_TOK("["), NT_TOK(RANGE), T_TOK("]"),  NULL},
	[10] =	(prod_tok*[]){IDENT_TOK(), T_TOK("-"), IDENT_TOK(), NULL}
	
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
int parse_table[7][9] =
{
	[REGEX] =		{0, 0, 0, 0, 0, 0, 0, 0, 0},
	[TERM] =		{2, 2, 2, 2, 2, 2, 2, 2, 2},
	[MORETERM] =	{-1, -1, -1, -1, -1, -1, -1, -1, -1},
	[FACTOR] =		{3, 3, 3, 3, 3, 3, 3, 3, 3},
	[BASE] =		{7, -1, -1, -1, 8, -1, 9, -1, -1},
	[BASE_SUFFIX] =	{-1, 4, 5, 6, -1, -1, -1, -1, -1},
	[RANGE] =		{10, -1, -1, -1, -1, -1, -1, -1, -1}
};

node *parse(lextok *lex_tokens_in)
{
	lex_tokens = lex_tokens_in;
	lex_tok = lex_tokens;

	node *tree = parse_nonterm(grammar_start_symbol);
	return tree;
}

//import_productions()
//generate_parse_table()

#define PARSER_FAILURE do {PARSER_STATUS = P_FAIL; return NULL;} while(0)
//#define PARSER_FAILURE do {PARSER_STATUS = FAIL; ptree_delete(root); return NULL;} while(0)
#define BAIL_IF_PARSER_FAILED if(PARSER_STATUS != P_OK) return NULL
//#define BAIL_IF_PARSER_FAILED if(PARSER_STATUS != P_OK) {ptree_delete(root); return NULL;}

node *parse_nonterm(nonterminal_type nt)
{
	printf("\nparsing nonterminal: %s\n", nt_strings[nt]);
	printf("\tlookahead at lex tok %d (%s)\n", lex_tok-lex_tokens, lex_tok->str);

	node *root = node_create(nt, NULL);
	int ci = 0;

	bool always_do_first, repeat;

	//look in the parse table, get the next production to apply
	int next_production = parse_table_lookup(nt);
	printf("\tapplying production %d\n", next_production);
	//if(next_production == -1)
	//	PARSER_FAILURE;
	prod_tok **rhs = productions[next_production];

	for(; *rhs; rhs++)
	{
		prod_tok *tok = *rhs, *next_tok = *(rhs+1), *second_tok;
		switch(tok->type)
		{
			case NONTERMINAL:

				if(next_tok && next_tok->type == EXPR) 
				{
					printf("\t\tnext token is expr (%c)\n", next_tok->expr);
					always_do_first = (next_tok->expr == '+');
					repeat = (next_tok->expr == '*' || next_tok->expr == '+');
					second_tok = *(rhs+2);

					if(!repeat)
					{
						/*printf("second tok is %s\n", second_tok);
						if(match(second_tok))*/
						if(!second_tok || match(second_tok))
							break;
					}
					while(1)
					{
						if(!always_do_first)
							//if(match(second_tok))
							if(!second_tok || match(second_tok))
								break;	//out of the while loop, not the case :|

						//
						root->children[ci++] = parse_nonterm(tok->nonterm);
						BAIL_IF_PARSER_FAILED;

						always_do_first = false;	//bad variable name lol
					}
				}
				else
				{
					root->children[ci++] = parse_nonterm(tok->nonterm);
					BAIL_IF_PARSER_FAILED;
				}
				break;

			case TERMINAL:
			case IDENT:
				if(match(tok))
				{
					//printf("found term/ident w string %s\n", lex_tok->str);
					//printf("in nonterm type %s\n", nt_strings[tok->type]);
					root->children[ci++] = node_create(tok->type, lex_tok->str);
					next();
					//printf("now lex_tok is %s\n", lex_tok->str);
				}
				else
					PARSER_FAILURE;
				break;

			//a node can represent a nonterminal or terminal, but we're getting amiguity from the 2 different types
			//of enums. the node should have a bool to distinguish
			case SEMACT:	root->children[ci++] = node_create(tok->type, tok->semact); break;
			
			case EXPR: 		break;		
		}
	}

	return root;
}

int parse_table_lookup(nonterminal_type nt)
{
	//lookup the column in the parse table
	bool col_found = false;
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
		col = 0;

	return parse_table[nt][col];
}

//the token must either be a terminal, or NULL (end of tokens)
bool match(prod_tok *tok)
{
	if(!tok)
		return (lex_tok == NULL);
	else if(tok->type == TERMINAL)
	{
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

//peek()

////////////////////////////////////////////////////////////////////

node *node_create(nonterminal_type type, const char *str)
{
    node *n = malloc(sizeof(*n));
    assert(n);

    n->type = type;
    for(int i=0; i<20; i++)
        n->children[i] = NULL;
    
    if(str)
    {
    	n->str = malloc(strlen(str)+1);
    	assert(n->str);
    	strcpy(n->str, str);
    }
    else
    	n->str = NULL;

    return n;
}

void ptree_print(node *pt)
{
	ptree_print_recursive(pt, 0);
}

void ptree_print_recursive(node *pt, int depth)
{
	for(int i=0; i<depth; i++)
		printf("  ");
	
	//printf(nonterm_strings[pt->type]);
	depth++;

	/*if(pt->c)
	{
		if(pt->type == NUMBER)  printf(" %d", pt->c);
		else                    printf(" %c", pt->c);
	}
	else
		printf(" %s", ptree_compose_string(pt));*/

	printf("(%s) ", nt_strings[pt->type]);
	if(pt->str)
		printf("%s", pt->str);
	putchar('\n');

	for(int ci=0; pt->children[ci]; ci++)
	{
		ptree_print_recursive(pt->children[ci], depth);
	}
}

