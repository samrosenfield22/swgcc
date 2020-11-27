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

static prod_tok *consume_token(char *s);
//static char *consume_token(char *s);
static prod_tok *consume_thing(char **s, char start, char end);
static prod_tok *consume_ident(char **s);
static int search_ident_name(char *s);

static prod_tok *add_classname(char *name, prod_tok_type type);
static int search_nonterm_name(char *name);
static int search_term_name(char *name);
static void print_production_rule(int i);

static void productions_to_parse_table(void);
static void mark_entries_for_nonterminal(nonterminal_type nt);
static void mark_parse_table(int nt, int alpha, int val);


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
    gg = (grammar){production_rules, nonterminal_names, terminal_names, vector_len(terminal_names)+ident_len};
        //vector_len(nonterminal_names), vector_len(production_rules)};

    productions_to_parse_table();


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
    //prod->rhs = calloc(80, sizeof(prod_tok *));
    prod->rhs = vector(prod_tok *, 0);

    //load first token into production's lhs
    prod->lhs = consume_token(bp)->nonterm;

    //load tokens into production's rhs
    //prod_tok **rhs_tok = prod->rhs;
    while(1)
    {
        prod_tok *p = consume_token(NULL);
        if(!p)
            break;
        //*rhs_tok++ = p;
        vector_append(prod->rhs, p);
    }

    //vector_append(prod->rhs, NULL);    
}

//consumes tokens until we reach the end of the string
static prod_tok *consume_token(char *s)
{
    static char *sp;
    if(s)
        sp = s;
    if(*sp == '\0')
        return NULL;

    prod_tok *t;

    //skip past whitespace and other chars we don't care about
    while(*sp==' ' || *sp=='\t' || *sp==':' || *sp=='=')
        sp++;
    
    switch(*sp)
    {
        case '<':   t = consume_thing(&sp, '<', '>'); break;  //nonterminal,
        case '"':   t = consume_thing(&sp, '"', '"'); break;   //
        case '{':   t = consume_thing(&sp, '{', '}'); break;  //semact

        case '+': case '*': case '?':
            t = add_classname((char[]){*sp, '\0'}, EXPR);
            sp++; break;

        //case ' ': case '\t': case ':': case '=':
        //    sp++; break;

        default:
            if(search_ident_name(sp) != -1)
                t = consume_ident(&sp);
            else
                {printf("found bad char in grammar:\n%s\n", sp); assert(0);}
            break;
    }

    return t;
}

static prod_tok *consume_thing(char **s, char start, char end)
{
    assert(**s == start);
    (*s)++;

    char *cn_end = strchr((*s)+1, end);    //was just (s, end)
    assert(cn_end);
    *cn_end = '\0';

    prod_tok_type ntype;
    switch(start)
    {
        case '<': ntype = NONTERMINAL; break;
        case '{': ntype = SEMACT; break;
        case '"': ntype = TERMINAL; break;
        default: assert(0);
    }

    prod_tok *t = add_classname(*s, ntype);
    *s = cn_end+1;
    return t;
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

static prod_tok *consume_ident(char **s)
{
    char *end;
    for(end=*s; !(*end==' ' || *end=='\t' || *end=='*' || *end=='+' || *end=='?' || *end=='\0'); end++);

    char temp = *end;
    *end = '\0';
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
        if(search_nonterm_name(name) == -1)
            vector_append(nonterminal_names, strdup(name));

        //nonterminal tokens (in the buffer) don't store their strings -- just an index to the nonterminal list
        t->nonterm = search_nonterm_name(name);
    }
    else if(ntype == TERMINAL)
    {
        if(search_term_name(name) == -1)
            vector_append(terminal_names, strdup(name));
    }

    return t;
}

static int search_nonterm_name(char *name)
{
    //for(int i=0; i<ntn_index; i++)
    for(int i=0; i<vector_len(nonterminal_names); i++)
    {
        if(strcmp(nonterminal_names[i], name)==0)
            return i;
    }
    return -1;
}

static int search_term_name(char *name)
{
    //for(int i=1; i<tn_index; i++)   //start at 1 to skip the NULL entry for idents
    //for(int i=0; i<tn_index; i++)
    for(int i=0; i<vector_len(terminal_names); i++)
    {
        if(strcmp(terminal_names[i], name)==0)
            return i;
    }
    return -1;
}

void dump_classnames(void)
{
    printf("\nnonterminals:\n-----------------\n");
    //for(int i=0; i<ntn_index; i++)
    for(int i=0; i<vector_len(nonterminal_names); i++)
    {
      printf("%s %d\n", nonterminal_names[i], i);
    }

    printf("\nterminals:\n-----------------\n");
    //for(int i=0; i<tn_index; i++)
    for(int i=0; i<vector_len(terminal_names); i++)
    {
      printf("%s %d\n", terminal_names[i], i);
    }
    printf("\n");
}

void dump_productions(grammar *g)
{
    printf("grammar length:\t%d\n", vector_len(g->rules)/*g->grammar_len*/);
    printf("alphabet length:\t%d\n", g->alphabet_len);
    printf("nonterm length:\t%d\n", vector_len(g->nonterminals)/*g->nonterm_len*/);

    for(int i=0; i<vector_len(g->rules)/*g->grammar_len*/; i++)
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
	int table_entries = gg.alphabet_len * vector_len(gg.nonterminals);//gg.nonterm_len;
	parse_table = malloc(table_entries * sizeof(*parse_table));
	assert(parse_table);
	for(int i=0; i<table_entries; i++)
		parse_table[i] = -1;

	//init array that tracks which productions have already been marked in the parse table
	production_marked = malloc(vector_len(gg.rules) /*gg.grammar_len*/ * sizeof(*production_marked));
	assert(production_marked);
	for(int i=0; i<vector_len(gg.rules)/*gg.grammar_len*/; i++)
		production_marked[i] = false;

	//mark entries for each production
	for(int i=0; i<vector_len(gg.rules)/*gg.grammar_len*/; i++)
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
	for(int i=0; i<vector_len(gg.rules)/*gg.grammar_len*/; i++)
	{
		if(gg.rules[i].lhs != nt)
			continue;
		//printf("\tmarking entries from production %d w lhs %d (nonterm %d)\n",
		//	i, productions[i].lhs, nt);

		//grab the first token of the production's rhs
		prod_tok **rhs = gg.rules[i].rhs;
		prod_tok *firsttok = *rhs;
        while(firsttok->ntype==SEMACT || firsttok->ntype==EXPR)
            firsttok++;
		switch(firsttok->ntype)
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

int find_parse_table_column(char *str, prod_tok_type ntype)
{
  int index;
  if(ntype == IDENT)
  {
      index = search_ident_name(str);
      assert(index != -1);
  }
  else if(ntype == TERMINAL)
  {
      index = search_term_name(str);
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
