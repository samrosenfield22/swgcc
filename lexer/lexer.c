

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "lexer.h"


//
/*static void make_identifier(const char *l, lextok *tp, lextok *default_tok);
static void make_decimal(const char *l, lextok *tp, lextok *default_tok);
static void make_hex(const char *l, lextok *tp, lextok *default_tok);
static void make_bin(const char *l, lextok *tp, lextok *default_tok);
static void make_op(const char *l, lextok *tp, lextok *default_tok);*/

static void lexer_build_all_regexes(void);

static void make_identifier(lextok *tp);
static void make_decimal(lextok *tp);
static void make_hex(lextok *tp);
static void make_bin(lextok *tp);
static void make_charlit(lextok *tp);


typedef struct regex_action_pair_s
{
    const char *name;           //matches to a user-defined name (such as an identifier or literal)
    const char *exp;            //regular expression
    nfa_model *model;           //nfa model for the regular expression (gets populated by lexer_initialize)
    lextok default_tok;         //default token for initializing a lexeme
    void (*action)(lextok *tp); //optional action for converting a token (ie converting a hex to dec)
} regex_action_pair;

#define LEXTOK_IDENT(id)    (lextok){NULL, true, id, 0, NULL}
#define LEXTOK_TERM         (lextok){NULL, false, 0, 0, NULL}

//regex table for all operators and keywords, and rules for identifiers and literals
regex_action_pair regex_table[] =
{
    

    //{"whatever", "; | { | } | = | , | \\&\\& | \\|\\| | \\^ | \\& | \\| | \\^ | == | != | < | > | <= | >= | << | >> | \\+ | - | \\* | \\/ | \\% | \\( | \\) | \\+= | -= | \\*= | \\/= | %= | <<= | >>= | \\&= | \\|= | \\^= | ++ | -- | ~",
    //NULL, LEXTOK_TERM, NULL},
    {"", "== | != | < | > | <= | >=", NULL, LEXTOK_TERM, NULL},
    {"", "\\+= | -= | \\*= | \\/= | %= | <<= | >>= | \\&= | \\|= | \\^= | ++ | --", NULL, LEXTOK_TERM, NULL},
    {"", "; | { | } | \\[ | \\] | \\( | \\) | = | , ", NULL, LEXTOK_TERM, NULL},
    {"", "\\&\\& | \\|\\| | \\^ | \\& | \\| | \\^ | ~", NULL, LEXTOK_TERM, NULL},
    {"", "<< | >> | \\+ | - | \\* | \\/ | \\%", NULL, LEXTOK_TERM, NULL},
    {"", "if | else | for | while | do | void", NULL, LEXTOK_TERM, NULL},

    {"type", "int|char|short|long", NULL, LEXTOK_IDENT(2), NULL},
    //{"id", "[a-z]+", NULL, LEXTOK_IDENT(0), make_identifier},
    {"id", "[A-Za-z]([A-Za-z0-9]|_)*", NULL, LEXTOK_IDENT(0), make_identifier},
    {"num", "[0-9]+", NULL, LEXTOK_IDENT(1), make_decimal},
    {"num", "0x[0-9A-Fa-f]+", NULL, LEXTOK_IDENT(1), make_hex},
    {"num", "0b(0|1)+", NULL, LEXTOK_IDENT(1), make_bin},
    {"num", "\'[ -~]\'", NULL, LEXTOK_IDENT(1), make_charlit}
    //{"num", "\\'[A-Za-z]\\'", NULL, LEXTOK_IDENT(1), make_charlit}
};

char *ident_table[] =
{
    "id",
    "num",
    "type"
};
int ident_len = 3;  //should get autogen'd


void lexer_initialize(void)
{
    nfa_builder_initialize();
    nfa_simulator_initialize();

    printf("building regexes... ");
    lexer_build_all_regexes();
    printf("done!\n");
}

static void lexer_build_all_regexes(void)
{
    for(int i=0; i<sizeof(regex_table)/sizeof(regex_table[0]); i++)
    {
        regex_table[i].model = regex_compile(regex_table[i].exp);
    }
}

//lex_token *lexer(const char *str)
lextok *lexer(const char *str)
{
    char buf[401], lexeme_buf[240];
    strncpy(buf, str, 400);
    char *bp = buf;

    lextok *toks = calloc(240, sizeof(*toks));
    assert(toks);
    lextok *tp = toks;

    int longest_match_ct, longest_match;
    while(*bp)
    {
        //swallow whitespace
        while(*bp==' ' || *bp=='\t' || *bp=='\n') bp++;
        if(*bp=='\0')
            break;

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

        //no matches found for a token
        //assert(longest_match != -1);
        if(longest_match == -1)
        {
            printf("no lexer match at %s\n", buf);
            for(int i=0; i<18 + (bp-buf); i++) putchar(' ');
            printf("^\n");
            free(toks);
            return NULL;
        }

        //break off the lexeme
        //printf("breaking off %d chars from \'%s\'\n", longest_match_ct, bp);
        strncpy(lexeme_buf, bp, longest_match_ct);
        lexeme_buf[longest_match_ct] = '\0';
        bp += longest_match_ct;

        //create a token for the match
        regex_action_pair *pair = &regex_table[longest_match];
        memcpy(tp, &pair->default_tok, sizeof(*tp));
        //tp->str = malloc(strlen(lexeme_buf)+1);
        //assert(tp->str);
        //strcpy(tp->str, lexeme_buf);
        tp->str = strdup(lexeme_buf);
        if(pair->action)
            pair->action(tp);
            //pair->action(lexeme_buf, tp, pair->default_tok);
        //regex_table[longest_match].action(lexeme_buf, tp);
        tp++;
    }

    //test -- dump all tokens
    

    return toks;
}

void lex_tokens_dump(lextok *lt)
{
    printf("\n--- lexer ---\n");
    for(lextok *l=lt; l->str; l++)
    {
        printf("%s ", l->str);
        if(l->is_ident)
        {
            //printf("(%s, val = %d)", ident_table[l->ident_id], l->val);
            printf("(%s", ident_table[l->ident_id]);
            if(strcmp(ident_table[l->ident_id], "num")==0)
                printf(", val = %d", l->val);
            printf(")");
        }
        putchar('\n');
    }

    printf("------------------\n\n");
}

/*static void make_identifier(const char *l, lextok *tp, lextok *default_tok)
{
    memcpy(tp, default_tok, sizeof(*tp));

    tp->type = IDENTIFIER;
    tp->opstr = NULL;
    tp->sym = symbol_search(l, SYM_ANY);
    if(!(tp->sym))
        tp->sym = symbol_create(l, SYM_IDENTIFIER);
}*/

static void make_identifier(lextok *tp)
{
    //printf("checking if identifier \"%s\" is already a symbol", tp->str);
    tp->sym = symbol_search(tp->str, SYM_ANY);
    //printf("table addr %p\n", tp->sym);
    if(tp->sym)
    {
        if(tp->sym->sym_type != SYM_IDENTIFIER)
            tp->is_ident = false;
    }
    else
        tp->sym = symbol_create(tp->str, SYM_IDENTIFIER, NULL);
}

static void make_decimal(lextok *tp)
{
    tp->val = strtol(tp->str, NULL, 10);
}

static void make_hex(lextok *tp)
{
    tp->val = strtol(tp->str, NULL, 16);

    free(tp->str);
    char buf[21];
    snprintf(buf, 20, "%d", tp->val);
    tp->str = strdup(buf);
}

static void make_bin(lextok *tp)
{
    tp->val = strtol((tp->str)+2, NULL, 2);

    free(tp->str);
    char buf[21];
    snprintf(buf, 20, "%d", tp->val);
    tp->str = strdup(buf);
}

static void make_charlit(lextok *tp)
{
    char c = tp->str[1];
    tp->val = c;

    free(tp->str);
    char buf[21];
    snprintf(buf, 20, "%d", tp->val);
    tp->str = strdup(buf);
}

//or maybe just a general make_symbol?
//static void make_type(lextok *tp)

/*static void make_op(const char *l, lextok *tp, lextok *default_tok)
{
    tp->type = OPERATOR;
    tp->opstr = malloc(strlen(l)+1);
    strcpy(tp->opstr, l);
    tp->sym = NULL;
}*/

