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


#define SOME_BIG_NUMBER (800)

static void build_production(char *bp);

static char *consume_token(char *s);
static char *consume_thing(char *s, char start, char end);
static char *consume_ident(char *s);
static int search_ident_name(char *s);

static void add_classname(char *name, prod_tok_type type);
static int search_nonterm_name(char *name);
static int search_term_name(char *name);
static void print_production_rule(int i);

static void productions_to_parse_table(void);
static void mark_entries_for_nonterminal(nonterminal_type nt);
static void mark_parse_table(int nt, int alpha, int val);

production_rule production_rules[SOME_BIG_NUMBER];
int pd_index;

char *nonterminal_names[SOME_BIG_NUMBER];
int ntn_index;

char *terminal_names[SOME_BIG_NUMBER];
int tn_index;

prod_tok token_buf[SOME_BIG_NUMBER];
int tbuf_index = 0;

grammar gg;

/*char *ident_table[] =
{
	"char",
    "num"
};
int ident_len = 2;  //for now
*/

grammar *load_grammar(const char *fname)
{
    FILE *fp = fopen(fname, "r");
    if(!fp) {printf("Failed to load grammar from file %s\n", fname);}
    assert(fp);

    //initialize indices
    tbuf_index = 0;
    ntn_index = 0;
    //tn_index = 1;   //terminal 0 is reserved for identifiers
    //terminal_names[0] = NULL;
    tn_index = 0;
    pd_index = 0;

    char buf[81];

    while(1)
    {
        fgets(buf, 80, fp);
        if(feof(fp))
            break;
        buf[strlen(buf)-1] = '\0';
        if(strlen(buf) == 0)
            continue;

        build_production(buf);
    }

    //load everything into the grammar structure
    //gg = (grammar){production_rules, nonterminal_names, terminal_names, tn_index, ntn_index, pd_index};
    gg = (grammar){production_rules, nonterminal_names, terminal_names, tn_index+ident_len, ntn_index, pd_index};

    productions_to_parse_table();


    return &gg;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void build_production(char *bp)
{
    printf("building production %s\n", bp);

    int first_token = tbuf_index;

    production_rules[pd_index].rhs = calloc(80, sizeof(prod_tok *));

    //scan the production text into tokens, load them into the token buffer
    while(*bp)
        bp = consume_token(bp);

    //load token into production's lhs
    production_rules[pd_index].lhs = token_buf[first_token].nonterm;

    //load tokens into production's rhs
    int r = 0;
    for(int i=first_token+1; i<tbuf_index; i++)
    {
        production_rules[pd_index].rhs[r] = &token_buf[i];
        r++;
    }
    //grammar[pd_index].rhs[r] = NULL;

    pd_index++;
}

static char *consume_token(char *s)
{
    
    switch(*s)
    {
        case '<':   s = consume_thing(s, '<', '>'); break;  //nonterminal,
        case '"':   s = consume_thing(s, '"', '"'); break;   //
        case '{':   s = consume_thing(s, '{', '}'); break;  //semact

        case '+': case '*': case '?':
            add_classname((char[]){*s, '\0'}, EXPR);
            s++; break;

        case ' ': case '\t': case ':': case '=':
            s++; break;

        default:
            if(search_ident_name(s) != -1)
                s = consume_ident(s);
            else
                {printf("found bad char in grammar:\n%s\n", s); assert(0);}
            break;
    }

    return s;
}

static char *consume_thing(char *s, char start, char end)
{
    assert(*s == start);
    s++;

    char *cn_end = strchr(s+1, end);    //was just (s, end)
    assert(cn_end);
    *cn_end = '\0';

    prod_tok_type type;
    switch(start)
    {
        case '<': type = NONTERMINAL; break;
        case '{': type = SEMACT; break;
        case '"': type = TERMINAL; break;
        default: assert(0);
    }
    add_classname(s, type); //save the string

    return cn_end+1;
}

static int search_ident_name(char *s)
{
    char *end;
    for(end=s; !(*end==' ' || *end=='\t' || *end=='*' || *end=='+' || *end=='?' || *end=='\0'); end++);
    //char temp = *end;
    //*end = '\0';

    char buf[81];
    strncpy(buf, s, end-s);
    buf[end-s] = '\0';

    int index = -1;
    //for(int i=0; i<sizeof(ident_table)/sizeof(ident_table[0]); i++)
    for(int i=0; i<ident_len; i++)
    {
      if(strcmp(buf, ident_table[i])==0)
      {
        index = i;
        break;
      }
    }

    //*end = temp;
    return index;
}

static char *consume_ident(char *s)
{
    char *end;
    for(end=s; !(*end==' ' || *end=='\t' || *end=='*' || *end=='+' || *end=='?' || *end=='\0'); end++);

    char temp = *end;
    *end = '\0';
    add_classname(s, IDENT);  //save the string
    *end = temp;

    return end;
}

static void add_classname(char *name, prod_tok_type type)
{
    //add name to the token buffer
    token_buf[tbuf_index].str = malloc(strlen(name)+1);
    assert(token_buf[tbuf_index].str);
    strcpy(token_buf[tbuf_index].str, name);
    token_buf[tbuf_index].type = type;

    //if the token is a nonterm or term, it gets added to those lists
    if(type == NONTERMINAL)
    {
        if(search_nonterm_name(name) == -1)
        {
            nonterminal_names[ntn_index] = malloc(strlen(name)+1);
            assert(nonterminal_names[ntn_index]);
            strcpy(nonterminal_names[ntn_index], name);


            ntn_index++;
        }

        //nonterminal tokens (in the buffer) don't store their strings -- just an index to the nonterminal list
        token_buf[tbuf_index].nonterm = search_nonterm_name(name);
    }
    else if(type == TERMINAL)
    {
        if(search_term_name(name) == -1)
        {
            terminal_names[tn_index] = malloc(strlen(name)+1);
            assert(terminal_names[tn_index]);
            strcpy(terminal_names[tn_index], name);

            tn_index++;
        }
    }

    tbuf_index++;
}

static int search_nonterm_name(char *name)
{
    for(int i=0; i<ntn_index; i++)
    {
        if(strcmp(nonterminal_names[i], name)==0)
            return i;
    }
    return -1;
}

static int search_term_name(char *name)
{
    //for(int i=1; i<tn_index; i++)   //start at 1 to skip the NULL entry for idents
    for(int i=0; i<tn_index; i++)
    {
        if(strcmp(terminal_names[i], name)==0)
            return i;
    }
    return -1;
}

void dump_classnames(void)
{
    printf("\nnonterminals:\n-----------------\n");
    for(int i=0; i<ntn_index; i++)
    {
      printf("%s %d\n", nonterminal_names[i], i);
    }

    printf("\nterminals:\n-----------------\n");
    for(int i=0; i<tn_index; i++)
    {
      printf("%s %d\n", terminal_names[i], i);
    }
    printf("\n");
}

void dump_productions(grammar *g)
{
    printf("grammar length:\t%d\n", g->grammar_len);
    printf("alphabet length:\t%d\n", g->alphabet_len);
    printf("nonterm length:\t%d\n", g->nonterm_len);

    for(int i=0; i<g->grammar_len; i++)
    {
        printf("production %d:\n\t", i);
        //printf("(%s)(nonterminal %d) ::= ", nonterminal_names[production_rules[i].lhs], production_rules[i].lhs);

        print_production_rule(i);

        printf("\n");
    }
}

static void print_production_rule(int i)
{
    printf("<%s> ::= ",
        nonterminal_names[gg.rules[i].lhs]);

    //char *tokname;
    for(prod_tok **ptp = production_rules[i].rhs; *ptp; ptp++)
    {
        if((*ptp)->type == NONTERMINAL)
            printf("<%s> ", gg.nonterminals[(*ptp)->nonterm]);
            //tokname = gg.nonterminals[(*ptp)->nonterm];
        else
            printf("%s ", (*ptp)->str);
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
	int table_entries = gg.alphabet_len * gg.nonterm_len;
	parse_table = malloc(table_entries * sizeof(*parse_table));
	assert(parse_table);
	for(int i=0; i<table_entries; i++)
		parse_table[i] = -1;

	//init array that tracks which productions have already been marked in the parse table
	production_marked = malloc(gg.grammar_len * sizeof(*production_marked));
	assert(production_marked);
	for(int i=0; i<gg.grammar_len; i++)
		production_marked[i] = false;

	//mark entries for each production
	for(int i=0; i<gg.grammar_len; i++)
		mark_entries_for_nonterminal(i);

  gg.parse_table = parse_table;

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
	for(int i=0; i<gg.grammar_len; i++)
	{
		if(gg.rules[i].lhs != nt)
			continue;
		//printf("\tmarking entries from production %d w lhs %d (nonterm %d)\n",
		//	i, productions[i].lhs, nt);

		//grab the first token of the production's rhs
		prod_tok **rhs = gg.rules[i].rhs;
		prod_tok *firsttok = *rhs;
        while(firsttok->type==SEMACT || firsttok->type==EXPR)
            firsttok++;
		switch(firsttok->type)
		{
			case SEMACT: case EXPR: assert(0); /*break;*/

			case TERMINAL:
			case IDENT:
        //this whole thing could just be
        //find_parse_table_column(firsttok)
        //if we changed find_parse_table_column() to take a prod_tok
				/*if(firsttok->type == IDENT)
        {
					//alpha_col = 0;
          alpha_col = search_ident_name(firsttok->str);
          assert(alpha_col != -1);
        }
				else
				{
					alpha_col = find_parse_table_column(firsttok->str) + ident_len;
					assert(alpha_col != -1);
				}*/

        alpha_col = find_parse_table_column(firsttok->str, firsttok->type);
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
		}
	}

	//mark this production as complete
	production_marked[nt] = true;
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

/*int find_parse_table_column(const char *symbol)
{
	//for(int col=1; col < sizeof(parse_table_columns)/sizeof(parse_table_columns[0]); col++)
	//for(int col=1; col<gg.alphabet_len; col++)
  for(int col=0; col<gg.alphabet_len; col++)
	{
		//if(strcmp(parse_table_columns[col], symbol) == 0)
		if(strcmp(gg.terminals[col], symbol) == 0)
		{
			return col;
		}
	}

	return -1;	//not found
}*/

int find_parse_table_column(char *str, prod_tok_type type)
{
  int index;
  if(type == IDENT)
  {
      index = search_ident_name(str);
      assert(index != -1);
  }
  else if(type == TERMINAL)
  {
      index = search_term_name(str);
      assert(index != -1);
      index += ident_len;
  }

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

	for(int i=0; i<gg.nonterm_len; i++)
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
