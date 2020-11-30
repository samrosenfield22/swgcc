

/*
recursive descent parser
in order to use this, the grammar.c library must first be used to load a grammar (read the productions and
generate a parse table)

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


pnode *parse(lextok *lex_tokens_in, bool verbose)
{
	PARSER_STATUS = P_OK;

	lex_tokens = lex_tokens_in;
	lex_tok = lex_tokens;

	//parse the token string, according to productions for the start symbol. generate the parse tree.
	pnode *tree = parse_nonterm(grammar_start_symbol);

	//error if something went wrong, or if we didn't reach the end of the lex token string
	if(PARSER_STATUS==P_FAIL || lex_tok->str != NULL)
	{
		if(verbose)
		{
			ptree_print(tree);
			printf("\n^^^ parse tree before failure\n\n");
		}

		//clean up
		return NULL;
	}
	else
		return tree;
}

#define PARSER_FAILURE do {PARSER_STATUS = P_FAIL; return root;} while(0)

//#define BAIL_IF_PARSER_FAILED if(PARSER_STATUS != P_OK) return NULL
//#define BAIL_IF_PARSER_FAILED if(PARSER_STATUS != P_OK) {ptree_traverse_dfs(root, NULL, node_delete, false); return NULL;}

pnode *parse_nonterm(nonterminal_type nt)
{
	//printf("\nparsing nonterminal: %s\n", gg.nonterminals[nt]);
	//printf("\tlookahead at lex tok %d (%s)\n", lex_tok-lex_tokens, lex_tok->str);

	pnode *root = pnode_create(true, nt, "", NULL);

	//look in the parse table, get the next production to apply
	int prod_index = parse_table_lookup(nt);
	if(prod_index == -1)
	{
		printf("parse error: unexpected \'%s\'\n", lex_tok->str);
		PARSER_FAILURE;
	}
	production_rule *prod = &gg.rules[prod_index];
	assert(prod->lhs == nt);
	//printf("\tapplying production %d\n", next_production);
	

	int tok_ct = vector_len(prod->rhs);
	//for(int i=0; i<tok_ct; i++)
	vector_foreach(prod->rhs, i)
	{
		prod_tok *tok = prod->rhs[i];
		prod_tok *next_tok = (i+1<tok_ct)? prod->rhs[i+1] : NULL;
		switch(tok->ntype)
		{
			case NONTERMINAL:
				if(!consume_nonterm(root, tok, next_tok))
					PARSER_FAILURE;
				break;

			case TERMINAL:
			case IDENT:
				if(!consume_term_or_ident(root, tok))
					PARSER_FAILURE;
				break;

			case SEMACT:	node_add_child(root, pnode_create(false, tok->ntype, tok->str, NULL)); break;
			case EXPR: 		break;
		}
	}

	//printf("done parsing %s\n\n", loaded_grammar->nonterminals[nt]);
	return root;
}

//bool consume_nonterm(node *root, int *ci, prod_tok *tok, prod_tok *next_tok)
bool consume_nonterm(pnode *root, prod_tok *tok, prod_tok *next_tok)
{
	//printf("consuming nonterm %s\n", loaded_grammar->nonterminals[tok->nonterm]);
	//printf("\tlookahead at lex tok %d (%s)\n", lex_tok-lex_tokens, lex_tok->str);

	bool always_do_first = true, repeat = false;

	if(next_tok && next_tok->ntype == EXPR)
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
		node_add_child(root, parse_nonterm(tok->nonterm));
		if(PARSER_STATUS != P_OK)
			return false;
	} while(repeat);

	return true;
}

bool consume_term_or_ident(pnode *root, prod_tok *tok)
{
	//printf("\t\tconsuming term/ident %s\n", tok->str);
	if(!match(tok))
	{
		//printf("failed to match token \'%s\' (type %s)\n", tok->str, t_strings[tok->type]);
		return false;
	}

	symbol *sym = (tok->ntype==IDENT)? lex_tok->sym : NULL;
	node_add_child(root, pnode_create(false, tok->ntype, lex_tok->str, sym));
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
	else if(tok->ntype == TERMINAL)
	{
		//printf("lex tok is a %s\n", lex_tok->is_ident)
		if(lex_tok->is_ident) return false;
		else return (strcmp(tok->str, lex_tok->str) == 0);
	}
	else if(tok->ntype == IDENT)
		return lex_tok->is_ident;
	else
		return false;
}

void next(void)
{
	lex_tok++;
}
