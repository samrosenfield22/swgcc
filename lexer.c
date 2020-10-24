

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "lexer.h"


//
static void make_identifier(const char *l, lex_token *tp);
static void make_decimal(const char *l, lex_token *tp);
static void make_hex(const char *l, lex_token *tp);
static void make_bin(const char *l, lex_token *tp);
static void make_op(const char *l, lex_token *tp);


//old shit
static char **lexer_scan(char *str);
static lex_token *lexer_tokenize(char **lexemes);

static bool is_numeric(const char *str);
static bool is_operator(const char *str);

typedef struct regex_action_pair_s
{
    const char *exp;
    nfa_model *model;
    void (*action)(const char *l, lex_token *tp);
} regex_action_pair;

regex_action_pair regex_table[] =
{
    //{"[A-Za-z]+[A-Za-z0-9]*", NULL, make_identifier},
    {"[a-z]+", NULL, make_identifier},
    {"[0-9]+", NULL, make_decimal},
    //{"0x[0-9A-Fa-f]+", NULL, make_hex},
    //{"0b(0|1)+", NULL, make_bin},
    {"= | , | \\&\\& | \\|\\| | \\^ | \\& | \\| | \\^ | == | != | < | > | <= | >= | << | >> | \\+ | - | \\* | \\/ | \\% | \\( | \\)", NULL, make_op}
    //still missing ? (ternary), +=, -=, *=...
    //{"\\+ | - | \\* | \\/ | \\% | \\&\\& | \\|\\| | \\^ | \\| | \\&", NULL, make_op},
};

void lexer_build_all_regexes(void)
{
    for(int i=0; i<sizeof(regex_table)/sizeof(regex_table[0]); i++)
    {
        regex_table[i].model = regex_compile(regex_table[i].exp);
    }
}

lex_token *lexer_new(const char *str)
{
    char buf[401], lexeme_buf[80];
    strncpy(buf, str, 400);
    char *bp = buf;

    lex_token *toks = calloc(80, sizeof(*toks));
    assert(toks);
    lex_token *tp = toks;

    int longest_match_ct, longest_match;
    while(*bp)
    {
        //swallow whitespace
        while(*bp==' ' || *bp=='\t') bp++;

        longest_match_ct = -1;
        longest_match = -1;

        //printf("matching string \'%s\'...\n", bp);
        for(int i=0; i<sizeof(regex_table)/sizeof(regex_table[0]); i++)
        {
            //try and match the regex against the current location in the string
            //printf("\tagainst regex %s\t\t", regex_table[i].exp);
            int moves = nfa_run(regex_table[i].model, bp);
            if(moves > longest_match_ct)
            {
                longest_match_ct = moves;
                longest_match = i;
            }
            //printf("%d moves\n", moves);
        }
        assert(longest_match != -1);

        //break off the lexeme
        //printf("breaking off %d chars from \'%s\'\n", longest_match_ct, bp);
        strncpy(lexeme_buf, bp, longest_match_ct);
        lexeme_buf[longest_match_ct] = '\0';
        bp += longest_match_ct;

        //create a token for the match
        regex_table[longest_match].action(lexeme_buf, tp);
        tp++;
    }

    dump_symbol_table();
    return toks;
}

static void make_identifier(const char *l, lex_token *tp)
{
    tp->type = IDENTIFIER;
    tp->opstr = NULL;
    tp->sym = symbol_search(l, SYM_ANY);
    if(!(tp->sym))
        tp->sym = symbol_create(l, SYM_IDENTIFIER);
}

static void make_decimal(const char *l, lex_token *tp)
{
    tp->type = LITERAL;
    tp->litval = strtol(l, NULL, 10);
    tp->opstr = NULL;
    tp->sym = NULL;
}

static void make_hex(const char *l, lex_token *tp)
{
    tp->type = LITERAL;
    tp->litval = strtol(l, NULL, 10);
    tp->opstr = NULL;
    tp->sym = NULL;
}

static void make_bin(const char *l, lex_token *tp)
{

}

static void make_op(const char *l, lex_token *tp)
{
    tp->type = OPERATOR;
    tp->opstr = malloc(strlen(l)+1);
    strcpy(tp->opstr, l);
    tp->sym = NULL;
}





/////////////////////////////////////////////////////////

lex_token *lexer(const char *str)
{
    char buf[401];
    strncpy(buf, str, 400);

    char **lexemes = lexer_scan(buf);
    //for(char **p=lexemes; *p; p++) printf("lexeme:\t%s\n", *p);
    lex_token *toks = lexer_tokenize(lexemes);
    dump_symbol_table();

    free(lexemes);
    return toks;
}

//for now just use spaces
static char **lexer_scan(char *str)
{
    char **lexemes = calloc(80, sizeof(*lexemes));
    assert(lexemes);
    char **lp = lexemes;

    char *c = strtok(str, " \t");
    while(c)
    {
        *lp++ = c;
        c = strtok(NULL, " \t");
    }

    return lexemes;
}


static lex_token *lexer_tokenize(char **lexemes)
{
    //any lexeme is of type: LITERAL, IDENT, OPERATOR
    lex_token *toks = calloc(80, sizeof(*toks));
    assert(toks);
    lex_token *tp = toks;

    for(char **l=lexemes; *l; l++)
    {
        //printf("tokenizing lexeme \'%s\'...\t ", *l);
        if(is_numeric(*l))
        {
            //printf("literal");
            tp->type = LITERAL;
            tp->litval = atoi(*l);
            tp->opstr = NULL;
            tp->sym = NULL;
        }
        else if(is_operator(*l))
        {
            //printf("operator");
            tp->type = OPERATOR;
            tp->opstr = malloc(strlen(*l)+1);
            strcpy(tp->opstr, *l);
            tp->sym = NULL;
        }
        else
        {
            //printf("identifier");
            tp->type = IDENTIFIER;
            tp->opstr = NULL;
            tp->sym = symbol_search(*l, SYM_ANY);
            if(!(tp->sym))
                tp->sym = symbol_create(*l, SYM_IDENTIFIER);
        }

        //printf("\n");
        tp++;
    }

    return toks;
}

static bool is_operator(const char *str)
{
    //printf("checking if operator");
    const char *operators[] = {"+", "-", "*", "/", "%", "^", "|", "&", "||", "&&", "==", "!=", ">=", "<=", "<<", ">>", "=", "(", ")", ",", NULL};

    for(const char **o=operators; *o; o++)
    {
        //printf("%s\n", *o);
        if(strcmp(str, *o)==0)
            return true;
    }

    return false;
}

static bool is_numeric(const char *str)
{
    //printf("checking if it's a number");
    for(const char *c=str; *c!='\0'; c++)
    {
        if(!isdigit(*c))
            return false;
    }

    return true;
}
