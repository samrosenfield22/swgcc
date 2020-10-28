

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
	CONFACTOR,
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
	[CONFACTOR] = "confactor",
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

const char *t_strings[] =
{
	[0] = "SHOULDNTSEETHIS",
	[IDENT] = "ident",
	[TERMINAL] = "terminal",
	[SEMACT] = "semact",
	[EXPR] = "expr"
};

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

typedef struct production_rule_s
{
	nonterminal_type lhs;
	prod_tok **rhs;
} production_rule;

//parse tree node
typedef struct node_s node;
struct node_s
{
    //nonterminal_type type;
		bool is_nonterminal;
		int type;

    node *children[20];
    char *str;
};

//
int *productions_to_parse_table(production_rule *productions);

node *parse(lextok *lex_tokens_in);
node *parse_nonterm(nonterminal_type nt);
int parse_table_lookup(nonterminal_type nt);
bool match(prod_tok *tok);
void next(void);
node *node_create(bool is_nonterminal, int type, const char *str);

void ptree_traverse_dfs(node *pt, void (*action)(node *pt, int arg), bool node_then_children);
void ptree_traverse_dfs_recursive(node *pt, void (*action)(node *pt, int arg), int depth, bool node_then_children);

void node_print(node *pt, int depth);
void semact_print(node *pt, int depth);
void node_delete(node *pt, int dummy);
//void ptree_print(node *pt);
//void ptree_print_recursive(node *pt, int depth);

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
	[10] =	{RANGE, (prod_tok*[]){IDENT_TOK(), T_TOK("-"), IDENT_TOK(), NULL}},
	[11] =	{CONFACTOR, (prod_tok*[]){NT_TOK(FACTOR), SEM_TOK("concat"), NULL}}
};
/*
prod_tok **productions[] =
{
	[0] =	(prod_tok*[]){NT_TOK(TERM), NT_TOK(MORETERM), EXPR_TOK('*'), NULL},
	//[0] =	(prod_tok*[]){NT_TOK(TERM), NT_TOK(MORETERM), NULL},
	[1] =	(prod_tok*[]){T_TOK("|"), NT_TOK(TERM), SEM_TOK("union"), NULL},
	[2] =	(prod_tok*[]){NT_TOK(FACTOR), NT_TOK(CONFACTOR), EXPR_TOK('*'), NULL},		//the concat sem_tok needs to be inside the repetition!
	[3] =	(prod_tok*[]){NT_TOK(BASE), NT_TOK(BASE_SUFFIX), EXPR_TOK('?'), NULL},
	[4] =	(prod_tok*[]){T_TOK("*"), SEM_TOK("0 or more"), NULL},
	[5] =	(prod_tok*[]){T_TOK("+"), SEM_TOK("1 or more"), NULL},
	[6] =	(prod_tok*[]){T_TOK("?"), SEM_TOK("0 or 1"), NULL},
	[7] =	(prod_tok*[]){IDENT_TOK(), SEM_TOK("push"), NULL},
	[8] =	(prod_tok*[]){T_TOK("("), NT_TOK(REGEX), T_TOK(")"), NULL},
	[9] =	(prod_tok*[]){T_TOK("["), NT_TOK(RANGE), T_TOK("]"),  NULL},
	[10] =	(prod_tok*[]){IDENT_TOK(), T_TOK("-"), IDENT_TOK(), NULL},
	[11] =	(prod_tok*[]){NT_TOK(FACTOR), SEM_TOK("concat"), NULL}

};
*/

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
int manual_parse_table[8][9] =
{
	[REGEX] =		{0, 0, 0, 0, 0, 0, 0, 0, 0},
	//[TERM] =		{2, 2, 2, 2, 2, 2, 2, 2, 2},
	[TERM] =		{2, -1, -1, -1, 2, -1, 2, -1, -1},
	[MORETERM] =	{-1, -1, -1, -1, -1, -1, -1, -1, 1},
	//[FACTOR] =		{3, 3, 3, 3, 3, 3, 3, 3, 3},
	[FACTOR] =		{3, -1, -1, -1, 3, -1, 3, -1, -1},
	[CONFACTOR] =	{11, -1, -1, -1, 11, -1, 11, -1, -1},
	[BASE] =		{7, -1, -1, -1, 8, -1, 9, -1, -1},
	[BASE_SUFFIX] =	{-1, 4, 5, 6, -1, -1, -1, -1, -1},
	[RANGE] =		{10, -1, -1, -1, -1, -1, -1, -1, -1}
};

int *parse_table = (int*)manual_parse_table;

int main(void)
{
	productions_to_parse_table(productions);
	//return 0;

	lextok *dummy = chars_to_substrings_lexer("(a(b|c))+|xyz|fg?h");

	//test lexer
	for(lextok *l=dummy; l->str; l++)
		printf("%s", l->str);

	void *tree = parse(dummy);
	if(!tree || lex_tok->str != NULL)
	{
		printf("\nparser failed!\n");
		return 0;
	}
	ptree_traverse_dfs(tree, node_print, true);
	ptree_traverse_dfs(tree, semact_print, true);
	return 0;
}

int alphabet_len = 8;	//not including idents
int nt_cnt = 8;
int *productions_to_parse_table(production_rule *productions)
{
	int table_entries = (alphabet_len+1) * nt_cnt;
	int *ptab = malloc(table_entries * sizeof(*ptab));
	assert(ptab);

	for(int i=0; i<table_entries; i++)
		ptab[i] = -1;



	return NULL;
}

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
//#define PARSER_FAILURE do {PARSER_STATUS = FAIL; return NULL;} while(0)
//#define BAIL_IF_PARSER_FAILED if(PARSER_STATUS != P_OK) return NULL
#define BAIL_IF_PARSER_FAILED if(PARSER_STATUS != P_OK) {ptree_traverse_dfs(root, node_delete, false); return NULL;}

node *parse_nonterm(nonterminal_type nt)
{
	printf("\nparsing nonterminal: %s\n", nt_strings[nt]);
	printf("\tlookahead at lex tok %d (%s)\n", lex_tok-lex_tokens, lex_tok->str);

	node *root = node_create(true, nt, NULL);
	int ci = 0;

	bool always_do_first, repeat;

	//look in the parse table, get the next production to apply
	int next_production = parse_table_lookup(nt);
	printf("\tapplying production %d\n", next_production);
	//if(next_production == -1)
	//	PARSER_FAILURE;

	for(prod_tok **rhs = productions[next_production].rhs; *rhs; rhs++)
	{
		prod_tok *tok = *rhs, *next_tok = *(rhs+1);//, *second_tok;
		//printf("\t\t%s\n", (tok->type==NONTERMINAL)? nt_strings[tok->nonterm] : t_strings[tok->type]);
		switch(tok->type)
		{
			case NONTERMINAL:

				if(next_tok && next_tok->type == EXPR)
				{
					//printf("\t\tnext token is expr (%c)\n", next_tok->expr);
					always_do_first = (next_tok->expr == '+');
					repeat = (next_tok->expr == '*' || next_tok->expr == '+');
					//second_tok = *(rhs+2);

					if(!repeat)
					{
						/*printf("second tok is %s\n", second_tok);
						if(match(second_tok))*/
						//if(!second_tok || match(second_tok))
						//	break;
						if(parse_table_lookup(tok->nonterm) == -1)
							break;
					}
					while(1)
					{
						if(!always_do_first)
							//if(match(second_tok))
							//if(!second_tok || match(second_tok))
							//	break;	//out of the while loop, not the case :|
							if(parse_table_lookup(tok->nonterm) == -1)
								break;

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
				//printf("\t\t\tlooking for terminal %s\n", tok->term);
				if(match(tok))
				{
					//printf("found term/ident w string %s\n", lex_tok->str);
					//printf("in term type %s\n", t_strings[tok->type]);
					root->children[ci++] = node_create(false, tok->type, lex_tok->str);
					if(lex_tok->is_ident) root->children[ci++] = node_create(false, SEMACT, lex_tok->str);	//just for testing
					next();
					//printf("now lex_tok is %s\n", lex_tok->str);
				}
				else
				{
					printf("failed to match token \'%s\' (type %s)\n", tok->term, t_strings[tok->type]);
					printf("lex tok is \'%s\'\n", lex_tok->str);
					PARSER_FAILURE;
				}
				break;

			//a node can represent a nonterminal or terminal, but we're getting amiguity from the 2 different types
			//of enums. the node should have a bool to distinguish
			case SEMACT:	root->children[ci++] = node_create(false, tok->type, tok->semact); /*root->children[ci++] = node_create(false, tok->type, lex_tok->str);*/ break;

			case EXPR: 		break;
		}
	}

	//printf("finished parsing %s\n\n", nt_strings[nt]);
	return root;
}

int parse_table_lookup(nonterminal_type nt)
{
	if(!lex_tok->str)
		return -1;

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

	//return parse_table[nt][col];

	int num_col = sizeof(parse_table_columns)/sizeof(parse_table_columns[0]);
	return parse_table[nt*num_col + col];
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

//peek()

////////////////////////////////////////////////////////////////////

node *node_create(bool is_nonterminal, int type, const char *str)
{
    node *n = malloc(sizeof(*n));
    assert(n);

		n->is_nonterminal = is_nonterminal;
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

void ptree_traverse_dfs(node *pt, void (*action)(node *pt, int arg), bool node_then_children)
{
		ptree_traverse_dfs_recursive(pt, action, 0, node_then_children);
}

void ptree_traverse_dfs_recursive(node *pt, void (*action)(node *pt, int depth), int depth, bool node_then_children)
{
		if(node_then_children)
			action(pt, depth);

		for(int ci=0; pt->children[ci]; ci++)
		{
			ptree_traverse_dfs_recursive(pt->children[ci], action, depth+1, node_then_children);
		}

		if(!node_then_children)
			action(pt, depth);
}

void node_print(node *pt, int depth)
{
	for(int i=0; i<depth; i++)
	{
		printf("  ");
	}
		if(pt->is_nonterminal == true)
			printf("(%s) ", nt_strings[pt->type]);
		else
			printf("(%s) ", t_strings[pt->type]);

		if(pt->str)
			printf("%s", pt->str);
		putchar('\n');
}

void semact_print(node *pt, int depth)
{
	if(!(pt->is_nonterminal) && pt->type == SEMACT)
		puts(pt->str);
}

void node_delete(node *pt, int dummy)
{
	free(pt);
}

/*
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

	if(pt->is_nonterminal == true)
		printf("(%s) ", nt_strings[pt->type]);
	else
		printf("(%s) ", t_strings[pt->type]);

	if(pt->str)
		printf("%s", pt->str);
	putchar('\n');

	for(int ci=0; pt->children[ci]; ci++)
	{
		ptree_print_recursive(pt->children[ci], depth);
	}
}
*/
