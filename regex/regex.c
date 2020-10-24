/*
http://matt.might.net/articles/parsing-regex-with-recursive-descent/

  <regex> ::= <term> '|' <regex>
            |  <term>

   <term> ::= { <factor> }

   <factor> ::= <base> { '*' }
            | <base> { '+' }
            | <base> { '?' }

   <base> ::= <char>
           |  '(' <regex> ')'
           | [range+]
    
    range ::= <char>-<char>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "regex.h"

enum tok_type_e
{
    REGEX,
    TERM,
    FACTOR,
    BASE,
    RANGE, 
    PRIM,

    CHARTOK,
    SEMACT
};

struct node_s
{
    tok_type type;
    node *children[80];
    //char *str;
    char c;
};

const char *rp = NULL;
int ri = 0;

//
static node *parse(const char *s);
static node *parse_regex(void);
static node *parse_term(void);
static node *parse_factor(void);
static node *parse_base(void);
static node *parse_range(void);

static node *pn_create(tok_type type, char c);
//static void pn_assign_string(node *n, char c);

static void index_advance(void);

//void print_ptree_recursive(node *pt, int depth);
//void ptree_extract_dfs_recursive(node *n);
void ptree_to_nfa_recursive(node *n);
//void ptree_compose_string_recursive(node *n);

/*void *compile_regex(const char *r)
{
    node *ptree = parse(r);
    return ptree;
}*/

nfa_model *regex_compile(const char *regex)
{
    void *parse_tree = parse(regex);
    //print_ptree(parse_tree);

    //printf("\n\n\n\n");
    //ptree_extract_dfs(parse_tree);
    nfa_model *n = ptree_to_nfa(parse_tree);
    //printf("\n%p\n", n);
    //nfa_dump(n);

    return n;
}

//generates a parse tree, returns the root
static node *parse(const char *s)
{
  rp = s;
  ri = 0;

  return parse_regex();
}

static node *parse_regex(void)
{
    //index_advance();
    while(rp[ri] == ' ')
        ri++;

    node *root = pn_create(REGEX, '\0');
    int ci = 0;

    root->children[ci++] = parse_term();
    if(rp[ri] == '|')
    {
        root->children[ci++] = pn_create(PRIM, rp[ri]);

        index_advance();
        root->children[ci++] = parse_regex();

        root->children[ci++] = pn_create(SEMACT, '|');
    }

    return root;
}

static node *parse_term(void)
{
    node *root = pn_create(TERM, '\0');
    int ci = 0;

    bool next_factor = false;

    while(rp[ri]!='|' && rp[ri]!=')' && rp[ri]!='\0')
    {
        root->children[ci++] = parse_factor();

        if(next_factor)
            root->children[ci++] = pn_create(SEMACT, '&');  //concat
        next_factor = true;
    }


    if(ci >= 80) {printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"); assert(0);}

    return root;
}

static node *parse_factor(void)
{
    node *root = pn_create(FACTOR, '\0');
    int ci = 0;

    root->children[ci++] = parse_base();
    while(rp[ri]=='*' || rp[ri]=='+' || rp[ri]=='?')
    {
        root->children[ci++] = pn_create(PRIM, rp[ri]);
        root->children[ci++] = pn_create(SEMACT, rp[ri]);

        index_advance();
    }

    return root;
}

static node *parse_base(void)
{
    node *root = pn_create(BASE, '\0');
    int ci = 0;

    switch(rp[ri])
    {
        case '(':
          ri++;
          root->children[ci++] = pn_create(PRIM, '(');
          root->children[ci++] = parse_regex();
          assert(rp[ri] == ')');
          root->children[ci++] = pn_create(PRIM, ')');
          index_advance();
        break;
        case '\\':
          ri++;
          root->children[ci++] = pn_create(PRIM, rp[ri]);
          root->children[ci++] = pn_create(CHARTOK, rp[ri]);
          index_advance();
        break;
        case '[':
            ri++;
            root->children[ci++] = pn_create(PRIM, '[');
            root->children[ci++] = parse_range();
            assert(rp[ri] == ']');
            root->children[ci++] = pn_create(PRIM, ']');
            index_advance();
            break;
        default:
          root->children[ci++] = pn_create(PRIM, rp[ri]);
          root->children[ci++] = pn_create(CHARTOK, rp[ri]);

          index_advance();
    }

    return root;
}

static node *parse_range(void)
{
    node *root = pn_create(RANGE, '\0');
    int ci = 0;

    char start, end;
    int union_ct = 0;

    start = rp[ri];
    index_advance();
    if(rp[ri] != '-') assert(0);
    while(rp[ri] == '-')
    {
        index_advance();
        end = rp[ri];
        printf("\tmaking range from %c to %c\n", start, end);
        for(char c=start; c<=end; c++)
        {
            root->children[ci++] = pn_create(PRIM, c);
            root->children[ci++] = pn_create(CHARTOK, c);
        }
        //root->children[ci++] = pn_create(PRIM, end);

        union_ct += (end-start);

        index_advance();
        if(rp[ri] == ']')
            break;
        start = rp[ri];
        index_advance();
        union_ct++;
    }

    for(int i=0; i<union_ct; i++)
        root->children[ci++] = pn_create(SEMACT, '|');

    return root;
}

static void index_advance(void)
{
    while(rp[ri] == ' ') ri++;
    ri++;
    while(rp[ri] == ' ') ri++;
}



static node *pn_create(tok_type type, char c)
{
    node *n = malloc(sizeof(*n));
    assert(n);

    n->type = type;
    for(int i=0; i<80; i++)
        n->children[i] = NULL;
    //n->str = NULL;
    n->c = c;

    return n;
}

//delet maybe
/*static void pn_assign_string(node *n, char c)
{
    n->c = c;
}*/

void *nfa_builder_stack;

void nfa_builder_initialize(void)
{
    nfa_builder_stack = stack_create(256, nfa_model);
}

nfa_model *ptree_to_nfa(node *n)
{
    stack_clear(nfa_builder_stack);
    start_new_nfa();

    ptree_to_nfa_recursive(n);
    //printf("stack depth: %d\n", stack_depth(nfa_builder_stack));
    return stack_pop(nfa_builder_stack);
}

void ptree_to_nfa_recursive(node *n)
{
    //printf("building...\n");

    for(int ci=0; n->children[ci]; ci++)
    {
        ptree_to_nfa_recursive(n->children[ci]);
    }

    nfa_model *na, *nb;
    if(n->type == SEMACT)
    {
        switch(n->c)
        {
            case ' ': case '(': case ')': break;
            case '|':
                nb = stack_pop(nfa_builder_stack);
                na = stack_pop(nfa_builder_stack);
                stack_push(nfa_builder_stack, nfa_union(na, nb));
                break;
            case '&':
                nb = stack_pop(nfa_builder_stack);
                na = stack_pop(nfa_builder_stack);
                stack_push(nfa_builder_stack, nfa_concatenate(na, nb));
                //printf("stack depth: %d\n", stack_depth(nfa_builder_stack));
                //printf("%s\n", stack_is_empty(nfa_builder_stack)? "empty":"not empty");
                break;
            case '*':
                na = stack_pop(nfa_builder_stack);
                stack_push(nfa_builder_stack, nfa_kleene(na));
                break;
            case '+':
                na = stack_pop(nfa_builder_stack);
                stack_push(nfa_builder_stack, nfa_plus(na));
                break;
            case '?':
                na = stack_pop(nfa_builder_stack);
                stack_push(nfa_builder_stack, nfa_question(na));
                break;
            default:
                break;
        }
    }
    else if(n->type == CHARTOK)
    {
        stack_push(nfa_builder_stack, nfa_create_from_char(n->c));
    }
}

/*void ptree_extract_dfs(node *n)
{
    printf("semantic actions\n------------------\n");
    ptree_extract_dfs_recursive(n);
}

void ptree_extract_dfs_recursive(node *n)
{
    for(int ci=0; n->children[ci]; ci++)
    {
        ptree_extract_dfs_recursive(n->children[ci]);
    }

    if(n->type == SEMACT)
        //printf("%c\n", n->c);
        switch(n->c)
        {
            case ' ': case '(': case ')': break;
            case '|': printf("pop 2 nfas; create their union; push it\n"); break;
            case '*': printf("pop nfa; create its kleene closure; push it\n"); break;
            case '+': printf("pop nfa; create (1 or more) closure; push it\n"); break;
            case '?': printf("pop nfa; create (0 or 1) closure; push it\n"); break;
            case '&': printf("pop 2 nfas; concatenate them; push it\n");  break;
            default:  printf("create nfa for %c; push it\n", n->c); break;
        }
}*/

/*void print_ptree(node *pt)
{
    print_ptree_recursive(pt, 0);

    printf("%s", ptree_compose_string(pt));
}

void print_ptree_recursive(node *pt, int depth)
{
    for(int i=0; i<depth; i++)
        printf("  ");

    switch(pt->type)
    {
        case REGEX:   printf("regex");  break;
        case TERM:    printf("term");   break;
        case FACTOR:  printf("factor"); break;
        case BASE:    printf("base");   break;
        case RANGE:   printf("range");  break;
        case PRIM:    printf("prim");   break;
        case CHARTOK: printf("chartok"); break;
        case SEMACT:  printf("semact"); break;
    }
    if(pt->c)
        printf(" %c", pt->c);
    else
        printf(" %s", ptree_compose_string(pt));
        //printf("(%s)", pt->str);
    putchar('\n');

    for(int ci=0; pt->children[ci]; ci++)
    {
        print_ptree_recursive(pt->children[ci], depth+1);
    }
}




char strbuf[240];
int strind = 0;

char *ptree_compose_string(node *n)
{
    strind = 0;
    memset(strbuf, '\0', 240);
    ptree_compose_string_recursive(n);
    return strbuf;
}

void ptree_compose_string_recursive(node *n)
{
    for(int ci=0; n->children[ci]; ci++)
    {
        ptree_compose_string_recursive(n->children[ci]);
    }

    if(n->c && n->type != SEMACT)
        strbuf[strind++] = n->c;
}
*/
