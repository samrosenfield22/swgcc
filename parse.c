/*
http://matt.might.net/articles/parsing-regex-with-recursive-descent/
https://craftinginterpreters.com/parsing-expressions.html
https://en.cppreference.com/w/c/language/operator_precedence


-------------- top level -------------
stmtlist -> {stmt*} | stmt
stmt -> stmt_val; | conditional
stmt_val -> comma | decl
decl -> type id = logical (, id = logical)*
comma -> assign (, assign)*
assign -> id (= | += | -= | *= | /= | %= | <<= | >>= | &= | |= | ^=) assign | logical
logical -> bitwise (&& bitwise)* | bitwise (|| bitwise)* | bitwise
bitwise -> equality (& equality)* | equality (| equality)* | equality (^ equality)* | equality
equality -> relational (== relational)* |  relational (!= relational)* |  relational
relational -> shift (> shift)* | shift (>= shift)* | shift (< shift)* | shift (<= shift)* | shift
shift -> sum (<< sum)* | sum (>> sum)* | sum
sum ->  prod (+ prod)* | prod (- prod)* | prod
prod -> base (* base)* | base (/ base)* | base (% base)* | base
base -> num | id | (comma)

conditional -> ifcond | ifelse | forloop...
forloop -> for(stmt_val; comma; comma) stmtlist
ifcond -> if(cond) stmtlist
-----------------------------------------

if takes 2 "arguments": the conditional, and the jump addr
if (expr) stmtlist {push jumpaddr} {if semact}
evaluate expr (result gets pushed)
push jump addr
if

if:
    pop addr
    pop expr


base -> num | id | (expr) | id(arglist)
arglist -> e | (, expr)*

decl -> type id initializer | type id(arglist) {stmtlist}
initializer -> e | = expr

instead of
decl -> type id (= comma)?
do
decl -> type id = logical (, id = logical)*

this seems to not work for statement:
  int c=5, d, e=6;
given the productions:
  stmt -> (comma | decl);
  decl -> type id (= comma)?
we expand:
  stmt -> decl -> comma (5,d,e=6) (breaks on e=6 because e isn't declared)





int val = 5,6,4,3;

cond -> if (expr) stmtlist
forloop -> for(stmt; comma; comma) stmtlist

int num = func(5);
int main(void) {}
if (thing) {}


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "parse.h"

#define PARSER_FAIL(reason)     {PARSER_STATUS = reason; return NULL;}


//const char *rp = NULL;
//int ri = 0;
lex_token *toks = NULL;
lex_token *tp;

int jmp_index = 0;

parser_status_type PARSER_STATUS = P_OK;

node *stmtlist_p(void);
node *stmt_p(void);
node *stmt_val_p(void);
node *decl_p(void);
node *comma_p(void);
node *assign_p(void);
node *logical_p(void);
node *bitwise_p(void);
node *equality_p(void);
node *relational_p(void);
node *shift_p(void);
node *sum_p(void);
node *prod_p(void);
node *base_p(void);

node *ifcond_p(void);
node *forloop_p(void);

static bool match_op(const char *op);
static bool match_op_peek(const char *op);
static bool match_variable(void);
//static bool match_variable_peek(void);
static void index_advance(void);
node *pn_create(ptree_tok_type type, int c);



void print_ptree_recursive(node *pt, int depth);
char *ptree_compose_string(node *n);
void ptree_compose_string_recursive(node *n);

/*void pn_assign_string(node *n, char c);
void ptree_extract_dfs(node *n);
void ptree_extract_dfs_recursive(node *n);
*/

node *(*grammar_start_nonterm)(void) = stmtlist_p;



//generates a parse tree, returns the root
/*node *parse(const char *s)
{
  rp = s;
  ri = 0;

  //return logical_p();
  return grammar_start_nonterm();
}*/

node *parse(lex_token *toks_in)
{
    toks = toks_in;
    tp = toks;

    PARSER_STATUS = P_OK;

    node *ptree = grammar_start_nonterm();
    switch(PARSER_STATUS)
    {
        case P_OK:
          assert(ptree);
          return ptree;
        case P_UNDECLARED:   printf("error: using undeclared variable\n"); break;
        case P_UNMATCHED_PAREN: printf("error: unmatched parentheses\n"); break;
        case P_ALREADY_DECLD: printf("error: redeclaring variable name\n"); break;
        //case P_DECL_NO_EQUALS: printf("error: declaration")
        case P_TYPE_NOT_FOLLOWED_BY_ID: printf("error: type name must be followed by identifier\n"); break;
        case P_STMTLIST_NO_MATCHING_CURLY: printf("error: no terminating curly brace\n"); break;
        case P_MISSING_SEMICOLON: printf("error: missing semicolon\n"); break;
        //case : printf(""); break;


        default: assert(0);
    }

    return NULL;
}

//stmtlist -> {stmt*} | stmt
node *stmtlist_p(void)
{
    node *root = pn_create(STMTLIST, '\0');
    int ci = 0;

    if(match_op("{"))
    {
        root->children[ci++] = pn_create(PRIM, '{');
        index_advance();

        //while(!match_op("}") && *(void**)(tp) != NULL)// && (*tp)!=(lex_token)(NULL))
        while(!match_op("}"))
        {

            root->children[ci++] = stmt_p();
            if(PARSER_STATUS != P_OK) return NULL;

            /*if(*(void**)(tp) != NULL)
            {
                printf("op is %s\n", tp->opstr);
                printf("%s\n", tp->sym->name);
                printf("\t\tnootnoot\n"); assert(0);
            }*/
        }

        if(!match_op("}"))
        {
            PARSER_STATUS = P_STMTLIST_NO_MATCHING_CURLY;
            return NULL;
            //PARSER_FAIL(P_STMTLIST_NO_MATCHING_CURLY)
        }
        root->children[ci++] = pn_create(PRIM, '}');
    }
    else
    {
        root->children[ci++] = stmt_p();
        if(PARSER_STATUS != P_OK) return NULL;
    }

    return root;
}

//stmt -> (stmt_val | conditional);
//stmt -> stmt_val; | conditional
node *stmt_p(void)
{
    node *root = pn_create(STMT_VAL, '\0');
    int ci = 0;

    if((tp->type == IDENTIFIER) && (tp->sym->type == SYM_OTHER_KW))
    {
        if(strcmp(tp->sym->name,"if")==0)
            root->children[ci++] = ifcond_p();
        else if(strcmp(tp->sym->name,"for")==0)
            root->children[ci++] = forloop_p();
        else assert(0);
    }
    else
    {
        root->children[ci++] = stmt_val_p();

        //eat the semicolon
        if(!match_op(";"))
        {
            PARSER_STATUS = P_MISSING_SEMICOLON;
            return NULL;
        }
        index_advance();
    }

    

    return root;
}

//if (expr) {push jumpaddr} {jumpz semact} stmtlist {jmp label}
node *ifcond_p(void)
{
    node *root = pn_create(STMT_VAL, '\0');
    int ci = 0;

    index_advance();

    if(!match_op("(")) {PARSER_STATUS = P_IFCOND_MISSING_PARENS; return NULL;}
    index_advance();

    root->children[ci++] = comma_p();
    if(PARSER_STATUS != P_OK) return NULL;

    if(!match_op(")")) {PARSER_STATUS = P_IFCOND_MISSING_PARENS; return NULL;}
    index_advance();

    //push the jump address (really a dummy instruction that gets replaced by the address of the jump label)
    root->children[ci++] = pn_create(JMP_ADDR, jmp_index);

    //{jumpz semact}
    root->children[ci++] = pn_create(SEMACT, 'j');

    root->children[ci++] = stmtlist_p();
    if(PARSER_STATUS != P_OK) return NULL;

    //push the jump label -- the address that gets jumped to if the condition == 0
    root->children[ci++] = pn_create(JMP_LABEL, jmp_index);

    jmp_index++;
    return root;
}

//forloop -> for(stmt_val; comma; comma) stmtlist
node *forloop_p(void)
{
    node *root = pn_create(STMT_VAL, '\0');
    int ci = 0;

    index_advance();

    if(!match_op("(")) {PARSER_STATUS = P_FORLOOP_MISSING_PARENS; return NULL;}
    index_advance();
    printf("matched (\n");

    //
    root->children[ci++] = stmt_val_p();
    if(PARSER_STATUS != P_OK) return NULL;
    printf("matched init\n");

    if(!match_op(";")) {PARSER_STATUS = P_MISSING_SEMICOLON; return NULL;}
    index_advance();
    printf("matched ;\n");

    root->children[ci++] = comma_p();
    if(PARSER_STATUS != P_OK) return NULL;
    printf("matched condition\n");

    if(!match_op(";")) {PARSER_STATUS = P_MISSING_SEMICOLON; return NULL;}
    index_advance();
    printf("matched ;\n");

    root->children[ci++] = comma_p();
    if(PARSER_STATUS != P_OK) return NULL;
    printf("matched increment\n");

    if(!match_op(")")) {PARSER_STATUS = P_FORLOOP_MISSING_PARENS; return NULL;}
    index_advance();

    //
    root->children[ci++] = stmtlist_p();
    if(PARSER_STATUS != P_OK) return NULL;
    

    return root;
}

//stmt_val -> (expr | decl)    //expr -> comma
node *stmt_val_p(void)
{
    node *root = pn_create(STMT_VAL, '\0');
    int ci = 0;
    
    //if the first token is a type, then the statement is a decl
    node *(*stmt_type)(void) = NULL;
    if((tp->type == IDENTIFIER) && (tp->sym->type == SYM_TYPE_KW))
        stmt_type = decl_p;
    else
        stmt_type = comma_p;

    root->children[ci++] = stmt_type();
    if(PARSER_STATUS != P_OK) return NULL;

    return root;
}

//decl -> type id (= comma)?
//decl -> type id = logical (, id = logical)*
node *decl_p(void)
{
    node *root = pn_create(DECL, '\0');
    int ci = 0;

    //get the type
    assert((tp->type == IDENTIFIER) && (tp->sym->type == SYM_TYPE_KW));
    //i don't know what to do w the type yet (Set the variable type, etc)
    
    while(1)
    {
        index_advance();

        //if(!(tp->type==IDENTIFIER && tp->sym->type==SYM_IDENTIFIER))  //can't have "int 5" or "int for"
        if(!match_variable())
        {
            PARSER_STATUS = P_TYPE_NOT_FOLLOWED_BY_ID;
            return NULL;
        }
        if(tp->sym->declared == true) //can't declare a var that's already been declared
        {
            PARSER_STATUS = P_ALREADY_DECLD;
            return NULL;
        }
        /*if(!match_op_peek("="))
        {
            PARSER_STATUS = P_DECL_NO_EQUALS;
            return NULL;
        }*/

        tp->sym->declared = true;   //declare it

        if(match_op_peek("="))
        {

            root->children[ci++] = pn_create(NUMBER, (int)(tp->sym)); //lvalue, so we use the address of the symbol
            root->children[ci++] = pn_create(PRIM, '=');

            //advance to the expression
            index_advance();
            index_advance();

            //
            root->children[ci++] = logical_p();
            if(PARSER_STATUS != P_OK) return NULL;

            root->children[ci++] = pn_create(SEMACT, '=');
        }
        else
            index_advance();

        if(!match_op(","))
            break;

    }

    return root;
}

//comma -> assign (, assign)*
node *comma_p(void)
{
    node *root = pn_create(COMMA, '\0');
    int ci = 0;

    root->children[ci++] = assign_p();
    if(PARSER_STATUS != P_OK) return NULL;

    while(match_op(","))
    {
        root->children[ci++] = pn_create(PRIM, ',');
        index_advance();
        root->children[ci++] = assign_p();
        if(PARSER_STATUS != P_OK) return NULL;
        root->children[ci++] = pn_create(SEMACT, ',');
    }

    return root;
}

//assign -> id (= | += | -= | *= | /= | %= | <<= | >>= | &= | |= | ^=) assign | logical
node *assign_p(void)
{
    node *root = pn_create(ASSIGN, '\0');
    int ci = 0;

    if(tp->type == IDENTIFIER && match_op_peek("="))  // = | += | -= | *= | /= | %= | <<= | >>= | &= | |= | ^=
    {
        //check if id is a variable, and if it's been declared
        if(!tp->sym->declared)
        {
            PARSER_STATUS = P_UNDECLARED;
            return NULL;
        }

        root->children[ci++] = pn_create(NUMBER, (int)(tp->sym)); //lvalue, so we use the address of the symbol
        //printf("parser: var w symbol at addr %p, value is %d\n", tp->sym, tp->sym->val);
        root->children[ci++] = pn_create(PRIM, '=');
        index_advance();
        index_advance();
        root->children[ci++] = assign_p();
        if(PARSER_STATUS != P_OK) return NULL;

        root->children[ci++] = pn_create(SEMACT, '=');
    }
    else
    {
        root->children[0] = logical_p();
        if(PARSER_STATUS != P_OK) return NULL;
    }

    return root;
}

node *logical_p(void)
{
    node *root = pn_create(LOGICAL, '\0');
    int ci = 0;

    root->children[ci++] = bitwise_p();
    if(PARSER_STATUS != P_OK) return NULL;

    while(match_op("||") || match_op("&&"))
    //while((rp[ri]=='|' && rp[ri+1]=='|') || (rp[ri]=='&' && rp[ri+1]=='&'))
    {
        char op = match_op("||")? 'O':'A';
        //char op = (rp[ri]=='|')? 'O':'A';
        root->children[ci++] = pn_create(PRIM, op);
        index_advance();
        //index_advance();
        root->children[ci++] = bitwise_p();
        if(PARSER_STATUS != P_OK) return NULL;

        root->children[ci++] = pn_create(SEMACT, op);
    }

    return root;
}

node *bitwise_p(void)
{
    node *root = pn_create(BITWISE, '\0');
    int ci = 0;

    root->children[ci++] = equality_p();
    if(PARSER_STATUS != P_OK) return NULL;

    while(match_op("&") || match_op("|") || match_op("^"))
    //while((rp[ri]=='&' || rp[ri]=='|' || rp[ri]=='^') && (rp[ri] != rp[ri+1]))
    {
        //char op = rp[ri];
        char op = *(tp->opstr);

        root->children[ci++] = pn_create(PRIM, op);
        index_advance();
        root->children[ci++] = equality_p();
        if(PARSER_STATUS != P_OK) return NULL;

        root->children[ci++] = pn_create(SEMACT, op);
    }

    return root;
}

node *equality_p(void)
{
    node *root = pn_create(EQUALITY, '\0');
    int ci = 0;

    root->children[ci++] = relational_p();
    if(PARSER_STATUS != P_OK) return NULL;

    while(match_op("==") || match_op("!="))
    //while((rp[ri]=='=' && rp[ri+1]=='=') || (rp[ri]=='!' && rp[ri+1]=='='))
    {
        char op = match_op("==")? 'E':'N';
        //char op = (rp[ri]=='=')? 'E':'N';

        root->children[ci++] = pn_create(PRIM, op);
        index_advance();

        root->children[ci++] = relational_p();
        if(PARSER_STATUS != P_OK) return NULL;

        root->children[ci++] = pn_create(SEMACT, op);
    }

    return root;
}

node *relational_p(void)
{
    node *root = pn_create(RELATIONAL, '\0');
    int ci = 0;

    root->children[ci++] = shift_p();
    if(PARSER_STATUS != P_OK) return NULL;

    while(match_op("<=") || match_op(">="))
    //while((strcmp(tp, "<=")==0) || (strcmp(tp, ">=")==0))
    //while((rp[ri]=='<' && rp[ri+1]=='=') || (rp[ri]=='>' && rp[ri+1]=='=')) //missing < and > operators
    {
        char op = match_op("<=")? 'l':'g';
        //char op = (rp[ri]=='<')? 'l':'g';

        root->children[ci++] = pn_create(PRIM, op);
        index_advance();

        root->children[ci++] = shift_p();
        if(PARSER_STATUS != P_OK) return NULL;

        root->children[ci++] = pn_create(SEMACT, op);
    }

    return root;
}

/*
any nonterminal w a left-associative operator of the form
a -> b (op b)*
is specified by:
node *(*nonterm)(void);
operator
node *(*nonterm)(void);
char id   //char that encodes the semantic action
*/

/*typedef struct left_assoc_nonterm_s left_assoc_nonterm;

struct left_assoc_nonterm_s
{
    left_assoc_nonterm *descent_func;
    node *(parse_func)(void);
    const char **op;
    char id;
};

node *left_assoc_nonterm_parse(left_assoc_nonterm *nt)
{
    node *root = pn_create(SHIFT, '\0');
    int ci = 0;

    if(nt->descent_func)
        node *child = left_assoc_nonterm_parse(nt->descent_func);
    else
        child = nt->parse_func();
    root->children[ci++] = child;

    while(1)
    {
        //check if we match one of the operators

    }

    while((rp[ri]=='<' && rp[ri+1]=='<') || (rp[ri]=='>' && rp[ri+1]=='>'))
    {
        char op = id;
        root->children[ci++] = pn_create(PRIM, op);
        index_advance();
        index_advance();
        root->children[ci++] = sum_p();
        root->children[ci++] = pn_create(SEMACT, op);
    }

    return root;
}*/

node *shift_p(void)
{
    node *root = pn_create(SHIFT, '\0');
    int ci = 0;

    root->children[ci++] = sum_p();
    if(PARSER_STATUS != P_OK) return NULL;

    while(match_op("<<") || match_op(">>"))
    //while((rp[ri]=='<' && rp[ri+1]=='<') || (rp[ri]=='>' && rp[ri+1]=='>'))
    {
        char op = match_op("<<")? 'L':'R';
        //char op = (rp[ri]=='<')? 'L':'R';

        root->children[ci++] = pn_create(PRIM, op);
        index_advance();
        //index_advance();
        root->children[ci++] = sum_p();
        if(PARSER_STATUS != P_OK) return NULL;
        root->children[ci++] = pn_create(SEMACT, op);
    }

    return root;
}

node *sum_p(void)
{
    node *root = pn_create(SUM, '\0');
    int ci = 0;

    root->children[ci++] = prod_p();
    if(PARSER_STATUS != P_OK) return NULL;

    while(match_op("+") || match_op("-"))
    //while(rp[ri] == '+' || rp[ri] == '-')
    {
        char op = *(tp->opstr);
        //char op = rp[ri];

        root->children[ci++] = pn_create(PRIM, op);
        index_advance();
        root->children[ci++] = prod_p();
        if(PARSER_STATUS != P_OK) return NULL;
        root->children[ci++] = pn_create(SEMACT, op);
    }

    return root;
}

node *prod_p(void)
{
    node *root = pn_create(PROD, '\0');
    int ci = 0;

    root->children[ci++] = base_p();
    if(PARSER_STATUS != P_OK) return NULL;

    while(match_op("*") || match_op("/") || match_op("%"))
    //while(rp[ri] == '*' || rp[ri] == '/' || rp[ri] == '%')
    {
        char op = *(tp->opstr);
        //char op = rp[ri];

        root->children[ci++] = pn_create(PRIM, op);
        index_advance();
        root->children[ci++] = base_p();
        if(PARSER_STATUS != P_OK) return NULL;
        root->children[ci++] = pn_create(SEMACT, op);
    }

    return root;
}

node *base_p(void)
{
    node *root = pn_create(BASE, '\0');
    int ci = 0;
    //int val;

    if(match_op("("))
    {
        index_advance();
        root->children[ci++] = pn_create(PRIM, '(');
        root->children[ci++] = comma_p();   //non grammar_start_nonterm(), this requires a semicolon, could be a decl...
        if(!match_op(")"))
        {
            PARSER_STATUS = P_UNMATCHED_PAREN;
            return NULL;
        }
        root->children[ci++] = pn_create(PRIM, ')');
        index_advance();
    }
    else if(tp->type == LITERAL)
    {
        root->children[ci++] = pn_create(NUMBER, tp->litval);
        index_advance();
    }
    else if(tp->type == IDENTIFIER)
    {
        //printf("using rvalue %d from identifier %s\n", tp->sym->val, tp->sym->name);
        if(tp->sym->declared == true)
        {
            

            //root->children[ci++] = pn_create(NUMBER, tp->sym->val);
            root->children[ci++] = pn_create(VARIABLE, (int)tp->sym);
            index_advance();
        }
        else
        {
            PARSER_STATUS = P_UNDECLARED;
            return NULL;
        }
    }
    else
    {
        printf("no good!\n"); 
        printf("%s\n", tp->sym->name);
        assert(0);
    }

    /*switch(rp[ri])
    {
        case '(':
          ri++;
          root->children[ci++] = pn_create(PRIM, '(');
          root->children[ci++] = grammar_start_nonterm();
          assert(rp[ri] == ')');
          root->children[ci++] = pn_create(PRIM, ')');
          index_advance();
        break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
          val = atoi(rp + ri);
          root->children[ci++] = pn_create(NUMBER, val);
          //root->children[ci++] = pn_create(SEMACT, val);
          index_advance();
          break;
        default:
          printf("no good! char is %c at index %d\n", rp[ri], ri);
          assert(0);
    }*/

    return root;
}



static bool match_op(const char *op)
{
    if(tp->type != OPERATOR)  return false;
    return (strcmp(op, tp->opstr)==0);
}

//checks the next token
static bool match_op_peek(const char *op)
{
    //bool match;
    tp++;
    //if(tp->type != OPERATOR)  match = false;
    //else match = (strcmp(op, tp->opstr)==0);
    bool match = match_op(op);
    tp--;
    return match;
}

static bool match_variable(void)
{
    return (tp->type==IDENTIFIER && tp->sym->type==SYM_IDENTIFIER);
}

/*static bool match_variable_peek(void)
{
    tp++;
    bool match = match_variable();
    tp--;
    return match;
}*/

static void index_advance(void)
{
    /*while(rp[ri] == ' ') ri++;
    ri++;
    while(rp[ri] == ' ') ri++;*/

    tp++;
}

node *pn_create(ptree_tok_type type, int c)
{
    node *n = malloc(sizeof(*n));
    assert(n);

    n->type = type;
    for(int i=0; i<20; i++)
        n->children[i] = NULL;
    //n->str = NULL;
    n->c = c;

    return n;
}





void print_ptree(node *pt)
{
    print_ptree_recursive(pt, 0);

    //printf("%s", ptree_compose_string(pt));
}

void print_ptree_recursive(node *pt, int depth)
{
    for(int i=0; i<depth; i++)
        printf("  ");

    switch(pt->type)
    {
        case STMTLIST:    printf("stmtlist"); break;
        case STMT:        printf("stmt");   break;
        case STMT_VAL:    printf("stmt_val"); break;
        case DECL:        printf("decl");   break;
        case COMMA:       printf("comma");   break;
        case ASSIGN:      printf("assign");   break;
        case LOGICAL:     printf("logical");   break;
        case BITWISE:     printf("bitwise");   break;
        case EQUALITY:    printf("equality");   break;
        case RELATIONAL:  printf("relational");   break;
        case SHIFT:       printf("shift");   break;
        case SUM:     printf("sum");     break;
        case PROD:    printf("prod");     break;
        case BASE:    printf("base");     break;
        case PRIM:    printf("prim");   break;

        case FORLOOP:   printf("forloop"); break;

        case NUMBER:  printf("prim");   break;
        case VARIABLE: printf("variable");  break;
        case SEMACT:  printf("semact"); break;
    }
    if(pt->c)
    {
        if(pt->type == NUMBER)  printf(" %d", pt->c);
        else                    printf(" %c", pt->c);
    }
    //else
    //    printf(" %s", ptree_compose_string(pt));
    putchar('\n');

    for(int ci=0; pt->children[ci]; ci++)
    {
        print_ptree_recursive(pt->children[ci], depth+1);
    }
}


char strbuf[80];
int strind = 0;

char *ptree_compose_string(node *n)
{
    strind = 0;
    memset(strbuf, '\0', 80);
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
