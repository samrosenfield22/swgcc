/*
grammar rules:

* nonterminals enclosed in <>, terminal strings enclosed in "", semantics enclosed in {}, identifiers and exprs (*,+,?) not enclosed
* the lhs nonterminal of the first production is the start symbol of the grammar
* the | symbol can NOT be used to specify multiple productions for a nonterminal
* each nonterminal must have either:
    only one production, or
    multiple productions, where each one's rhs starts with a (distinct) terminal/identifier
*

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "grammar.h"

const char *tm_strings[] =
{
    [NONTERMINAL] = "nonterminal",
    [IDENT] = "ident",
    [TERMINAL] = "terminal",
    [SEMACT] = "semact",
    [EXPR] = "expr"
};


static void build_production(char *bp);

static prod_tok *consume_token(char *s);
static prod_tok *consume_thing(char **s, char start, char end, prod_tok_type ntype);
static prod_tok *consume_ident(char **s);

static prod_tok *add_classname(char *name, prod_tok_type type);
static void print_production_rule(int i);

static void productions_to_parse_table(void);
static void mark_entries_for_nonterminal(nonterminal_type nt);
static void mark_parse_table(int nt, int alpha, int val);

void dump_parse_table_entry(int *ptab, char *ntname);


//vectors
production_rule *production_rules;
char **nonterminal_names;
char **terminal_names;

//main grammar structure
grammar gg;


grammar *load_grammar(const char *fname)
{
    FILE *fp = fopen(fname, "r");
    if(!fp) {printf("Failed to load grammar from file %s\n", fname);}
    assert(fp);

    //initialize vector structures
    nonterminal_names = vector(*nonterminal_names, 0);
    terminal_names = vector(*terminal_names, 0);
    production_rules = vector(*production_rules, 0);

    char buf[241];

    while(1)
    {
        fgets(buf, 240, fp);
        if(feof(fp))
            break;
        buf[strlen(buf)-1] = '\0';
        if(strlen(buf) == 0)
            continue;

        build_production(buf);
    }

    //load everything into the grammar structure
    gg = (grammar){production_rules, nonterminal_names, terminal_names,
        vector_len(terminal_names)+ident_len, NULL};
    //dump_productions(&gg); getchar();

    //build the parse table
    productions_to_parse_table();
    //dump_parse_table_entry(gg.parse_table, "dcltor"); getchar();

    return &gg;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void build_production(char *bp)
{
    //printf("building production %s\n", bp);
    if(bp[0] == ';')    //bnf comment
        return;

    vector_inc(&production_rules);
    production_rule *prod = &vector_last(production_rules);
    prod->rhs = vector(prod_tok *, 0);

    //load first token into production's lhs
    prod->lhs = consume_token(bp)->nonterm;

    //load tokens into production's rhs
    while(1)
    {
        prod_tok *p = consume_token(NULL);
        if(!p)
            break;
        vector_append(prod->rhs, p);
    }  
}

//consumes tokens until we reach the end of the string
static prod_tok *consume_token(char *s)
{
    static char *sp;
    if(s)               sp = s;
    if(*sp == '\0')     return NULL;

    //skip past whitespace and other chars we don't care about
    while(*sp==' ' || *sp=='\t' || *sp==':' || *sp=='=')
        sp++;
    
    switch(*sp)
    {
        case '<':                       return consume_thing(&sp, '<', '>', NONTERMINAL);
        case '"':                       return consume_thing(&sp, '"', '"', TERMINAL);
        case '{':                       return consume_thing(&sp, '{', '}', SEMACT);
        case '+': case '*': case '?':   return add_classname((char[]){*sp++, '\0'}, EXPR);
        default:                        return consume_ident(&sp);
    }
}

static prod_tok *consume_thing(char **s, char start, char end, prod_tok_type ntype)
{
    //get start
    assert(**s == start);
    (*s)++;

    //get end, replace it with nul
    char *cn_end = strchr((*s)+1, end);    //was just (s, end)
    assert(cn_end);
    *cn_end = '\0';

    prod_tok *t = add_classname(*s, ntype);
    *s = cn_end+1;
    return t;
}

static prod_tok *consume_ident(char **s)
{
    //nul-terminate after the ident
    char *end = firstchr(*s, " \t*+?");
    char temp = *end;
    *end = '\0';

    if(array_search_str(ident_table, ident_len, *s) == -1)
    {
        printf("found bad char in grammar:\n%s\n", *s);
        assert(0);
    }

    prod_tok *t = add_classname(*s, IDENT);  //save the string
    *end = temp;

    *s = end;
    return t;
}

static prod_tok *add_classname(char *name, prod_tok_type ntype)
{
    //allocate/initialize the production token
    prod_tok *t = malloc(sizeof(*t));
    t->str = strdup(name);
    t->ntype = ntype;

    //if the token is a nonterm or term, it gets added to those lists
    if(ntype == NONTERMINAL)
    {
        int ntindex = vector_search_str(nonterminal_names, name);
        if(ntindex == -1)
            vector_append(nonterminal_names, strdup(name));

        //nonterminal tokens (in the buffer) don't store their strings -- just an index to the nonterminal list
        t->nonterm = vector_search_str(nonterminal_names, name);
    }
    else if(ntype == TERMINAL)
    {
        if(vector_search_str(terminal_names, name) == -1)
            vector_append(terminal_names, strdup(name));
    }

    return t;
}

void dump_classnames(void)
{
    printf("\nnonterminals:\n-----------------\n");
    vector_foreach(nonterminal_names, i)
        printf("%s %d\n", nonterminal_names[i], i);

    printf("\nterminals:\n-----------------\n");
    vector_foreach(terminal_names, i)
        printf("%s %d\n", terminal_names[i], i);

    printf("\n");
}

void dump_productions(grammar *g)
{
    printf("grammar length:\t%d\n", vector_len(g->rules));
    printf("alphabet length:\t%d\n", g->alphabet_len);
    printf("nonterm length:\t%d\n", vector_len(g->nonterminals));

    vector_foreach(g->rules, i)
    {
        printf("production %d:\n\t", i);
        //printf("(%s)(nonterminal %d) ::= ", nonterminal_names[production_rules[i].lhs], production_rules[i].lhs);

        print_production_rule(i);

        printf("\n");
    }
}

static void print_production_rule(int n)
{
    printf("<%s> ::= ",
        nonterminal_names[gg.rules[n].lhs]);

    //char *tokname;
    //for(prod_tok **ptp = production_rules[i].rhs; *ptp; ptp++)
    for(int i=0; i<vector_len(production_rules[n].rhs); i++)
    {
        prod_tok *ptp = production_rules[n].rhs[i];
        if((ptp)->ntype == NONTERMINAL)
            printf("<%s> ", gg.nonterminals[(ptp)->nonterm]);
            //tokname = gg.nonterminals[(*ptp)->nonterm];
        else if(ptp->ntype == SEMACT)
            printf("{%s} ", ptp->str);
        else
            printf("%s ", (ptp)->str);
            //tokname = (*ptp)->str;

        //printf("(%s)(%s) ", tokname, tm_strings[(*ptp)->type]);
    }
}

//////////////////////////////////////////////////////////////////

int *parse_table;
bool *production_marked;
#define table_entry(nt, alphabet_index) (nt*gg.alphabet_len + alphabet_index)
static void productions_to_parse_table(void)
{
	//init parse table structure
	int table_entries = gg.alphabet_len * vector_len(gg.nonterminals);
	parse_table = malloc(table_entries * sizeof(*parse_table));
	assert(parse_table);
	for(int i=0; i<table_entries; i++)
		parse_table[i] = -1;

	//init array that tracks which productions have already been marked in the parse table
	production_marked = malloc(vector_len(gg.rules) * sizeof(*production_marked));
	assert(production_marked);
    vector_foreach(gg.rules, i)
		production_marked[i] = false;

	//mark entries for each production
    vector_foreach(gg.rules, i)
		mark_entries_for_nonterminal(i);

    gg.parse_table = parse_table;
    free(production_marked);
}

/*
	for each production for that nonterminal
		if the first (non-semantic) element of the rhs is a terminal/ident
			ptab[table_entry(nt, term)] = that production's id
		else if it's a nonterminal
			make sure that's the ONLY production for that nonterminal (else error)
			recurse on that nonterminal
*/
static void mark_entries_for_nonterminal(nonterminal_type nt)
{
	if(production_marked[nt] == true)
		return;

	int alpha_col;

	//iterate through all productions for this nonterminal
	for(int i=0; i<vector_len(gg.rules); i++)
	{
		if(gg.rules[i].lhs != nt)
			continue;

        

		//grab the first token of the production's rhs (skipping semacts and exprs)
        prod_tok *firsttok = NULL;
        vector_foreach(gg.rules[i].rhs, ptok)
        {
            prod_tok *tok = gg.rules[i].rhs[ptok];
            if(!(tok->ntype==SEMACT || tok->ntype==EXPR))
            {
                firsttok = tok;
                break;
            }
        }
        assert(firsttok);

		/*prod_tok **rhs = gg.rules[i].rhs;
		prod_tok *firsttok = *rhs;
        while(firsttok->ntype==SEMACT || firsttok->ntype==EXPR)
        {
            printf("production for \'%s\' leads with semact/expr %s\n", gg.nonterminals[nt], firsttok->str);
            problem_child = true;
            getchar();
            firsttok++;
        }*/
		switch(firsttok->ntype)
		{
			case SEMACT: case EXPR: assert(0); /*break;*/

			case TERMINAL:
			case IDENT:

                alpha_col = find_parse_table_column(firsttok->str, firsttok->ntype);
                assert(alpha_col != -1);

				//parse_table[table_entry(nt, alpha_col)] = i;
                mark_parse_table(nt, alpha_col, i);
				break;

			case NONTERMINAL:

    			//make sure that's the ONLY production for that nonterminal
    			//for now we're just assuming that's the case...
    			//...or does this code work?
    			/*for(int j=i+1; j<gg.grammar_len; j++)
    				if(gg.rules[j].lhs == nt)
    					assert(0);*/

    			//recurse on that nonterminal
    			mark_entries_for_nonterminal(firsttok->nonterm);

    			//copy all the ones that aren't -1
    			for(int j=0; j<gg.alphabet_len; j++)
    			{
    				if(parse_table[table_entry(firsttok->nonterm, j)] != -1)
    					//parse_table[table_entry(nt, j)] = i;
                        mark_parse_table(nt, j, i);
    			}
                break;

            default:
                printf("crashed (invalid token type %d) in production:\n", firsttok->ntype);
                print_production_rule(i);
                assert(0);
		}
	}

	//mark this production as complete
	production_marked[nt] = true;
}

void dump_parse_table_entry(int *ptab, char *ntname)
{
    printf("productions for \'%s\':\n", ntname);

    int ntindex = vector_search_str(nonterminal_names, ntname);
    if(ntindex == -1)
    {
        printf("can't dump parse table entry for %s, not found!\n", ntname);
        exit(-1);
    }

    int index = ntindex;
    for(int j=0; j<gg.alphabet_len; j++)
    {
        int trans = ptab[index*gg.alphabet_len+j];
        if(trans == -1)
            continue;

        char *term = (j<ident_len)? ident_table[j] : gg.terminals[j-ident_len];
        //int becomes = gg.rules[trans].rhs[0]->nonterm;
        //printf("\t%s : %s (%d)\n", term, gg.nonterminals[becomes], trans);
        printf("\t%s :: ", term);
        //print_production_rule(gg.rules[trans].rhs[0]->nonterm);
        vector_foreach(gg.rules[trans].rhs, tok)
        {
            if(gg.rules[trans].rhs[tok]->ntype == NONTERMINAL)
            {
                int first_rhs_nt = gg.rules[trans].rhs[tok]->nonterm;
                printf("%s\n", gg.nonterminals[first_rhs_nt]);
                break;
            }
        }
    }
}

static void mark_parse_table(int nt, int alpha, int val)
{
    int index = table_entry(nt, alpha);
    if(parse_table[index] != -1)  //already marked, there's a problem with the grammar
    {
        printf("failure constructing parse table: mulitple parse paths for nonterminal=%s, lookahead=%s\n",
            gg.nonterminals[nt], (alpha<ident_len)? ident_table[alpha] : gg.terminals[alpha-ident_len]);

        printf("\t%d:\t", parse_table[index]); print_production_rule(parse_table[index]); printf("\n");
        printf("\t%d:\t", val);                print_production_rule(val); printf("\n");
        assert(0);
    }
    parse_table[index] = val;
}

int find_parse_table_column(char *str, prod_tok_type ntype)
{
  int index;
  if(ntype == IDENT)
  {
      index = array_search_str(ident_table, ident_len, str);
      assert(index != -1);
  }
  else if(ntype == TERMINAL)
  {
      index = vector_search_str(terminal_names, str);
      if(index == -1)
      {
        printf("\'%s\' is a lex terminal but it's not in the parser/grammar!\n", str);
        assert(0);
      }
      index += ident_len;
  }
  else    
      assert(0);

  return index;
}

void dump_parse_table(int *pt)
{
	printf("\n\n\t\t");

	//for(int i=0; i<gg.alphabet_len; i++)
  for(int i=0; i<ident_len; i++)
    printf("%s\t", ident_table[i]);
  for(int i=ident_len; i<gg.alphabet_len; i++)
		printf("%s\t", gg.terminals[i-ident_len]);
	putchar('\n');

	for(int i=0; i<vector_len(gg.nonterminals)/*gg.nonterm_len*/; i++)
	{
		int cct = printf("%s", gg.nonterminals[i]);
		for(int i=cct; i<16; i++)
			putchar(' ');
		for(int j=0; j<gg.alphabet_len; j++)
			printf("%d\t", pt[i*gg.alphabet_len+j]);
		putchar('\n');
	}
	printf("\n\n");
}
