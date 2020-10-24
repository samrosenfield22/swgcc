

#ifndef PARSE_H_
#define PARSE_H_

#include "lexer.h"

typedef enum ptree_tok_type_e
{
    STMT,
    DECL,
    COMMA,
    ASSIGN,
    LOGICAL,
    BITWISE,
    EQUALITY,
    RELATIONAL,
    SHIFT,
    SUM,
    PROD,
    BASE,

    PRIM,
    NUMBER,
    SEMACT
} ptree_tok_type;

typedef struct node_s node;
struct node_s
{
    ptree_tok_type type;
    node *children[20];
    //char *str;
    int c;
};

typedef enum parser_status_type_e
{
    P_OK,
    P_UNDECLARED,
    P_UNMATCHED_PAREN,
    P_ALREADY_DECLD,
    P_DECL_NO_EQUALS,
    P_TYPE_NOT_FOLLOWED_BY_ID
} parser_status_type;



//node *parse(const char *s);
node *parse(lex_token *toks);

void print_ptree(node *pt);

#endif //PARSE_H_
