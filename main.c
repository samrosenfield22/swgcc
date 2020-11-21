//  gcc -o main.exe main.c regex.c ../../swglib/automata/nfa/nfa_build.c ../../swglib/automata/nfa/nfa_run.c ../../swglib/structures/stack/stack.c $(FLAGS)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lexer/lexer.h"
#include "parser/recdesc.h"
#include "interpreter.h"
#include "symbol.h"
#include "utils/printcolor.h"


void test_compiler(void);


int main(void)
{

    symbol_table_initialize();
    lexer_initialize();

    load_grammar("parser/c_grammar.txt");
    //grammar *g = load_grammar("parser/c_grammar.txt");
    //dump_classnames();
    //dump_parse_table(g->parse_table);

    //run test cases
    //test_compiler();

    //interpret forever
    launch_interpreter();

    
    return 0;
}

/*
things to test:

decl
multiple decl w comma
comma exprs
misc logical/arith exprs
address/dereference     (p=&a,0)
"complicated" lexemes (funny idents, bin, hex, char lits)
blocks
if
while
do while
nested conditionals

code that should fail:
bad lexeme
bad syntax
redecl variable
bad expr in lval context

*/
typedef struct test_case_s {char *src; int exp_status; int exp_val;} test_case;
test_case test_cases[] =
{
    {"int a=5;", PASS, 5},
    {"int b,c=1,p,pp,n;", PASS, 0},
    {"b=1+5,4,2<<4;", PASS, 32},
    {"3*(1<<5/3) + (26^12);", PASS, (3*(1<<5/3) + (26^12))},
    {"p=&a,0;", PASS, 0},   //use the comma so this evalutates to 0 -- we aren't checking the value of &a
    {"++(*p);", PASS, 6},
    {"*(p+8);", PASS, 1},
    {"pp=&p,0;", PASS, 0},  //aren't checking
    {"**pp = b;", PASS, 6},
    {"b;", PASS, 6},
    {"int a__1 = 0xFF;", PASS, 255},
    {"a__1 = '!';", PASS, '!'},
    {"a__1 = 0b11011011;", PASS, 0b11011011},
    {"{a=8; b=11;}", PASS, 11},
    {"a;", PASS, 8},
    {"if(a-7) 22;", PASS, 22},
    {"if(a-8) 22;", PASS, 0},
    {"{while(a) {c*=a; a--;} c;}", PASS, 40320},
    //do while {";", PASS, },
    {"{while(c > 40220) {if(c & 0b1) n++; c--;} n;}", PASS, 50},

    {"int fn(void) {return 11<<1;}", PASS, 0},
    {"fn() + 5;", PASS, 27},
    {"int afact(void) {b=1; while(a) {b*=a; a--;} return b;}", PASS, 0},
    {"{a=5; afact();}", PASS, 120},

    //code that should fail
    {"int 9a;", LEX_FAIL, 0},
    {"a = !!!;", LEX_FAIL, 0},
    {"a + -;", PARSE_FAIL, 0},
    {"int int;", PARSE_FAIL, 0},
    {"a = ((5);", PARSE_FAIL, 0},
    {"= a + 1;", PARSE_FAIL, 0},
    {"int p;", SEMANTIC_FAIL, 0},
    {"int q = q;", SEMANTIC_FAIL, 0},
    {"a = notdecld;", SEMANTIC_FAIL, 0},
    {"5 = a;", SEMANTIC_FAIL, 0},
    //{";", , 0},
    
};

void test_compiler(void)
{
    printf("\nrunning test cases..... ");

    const char *fail_strings[] = {"", "lexer", "parser", "semantic"};
    int res;
    for(int i=0; i<sizeof(test_cases)/sizeof(test_cases[0]); i++)
    {
        int status = interpreter(&res, SILENT, test_cases[i].src);
        if(status != test_cases[i].exp_status)
        {
            if(status == PASS)
            {
                printfcol(RED_FONT, "\nfailed test case %d (%s fail):\n%s\n",
                    i, fail_strings[test_cases[i].exp_status], test_cases[i].src);
                exit(-1);
            }
        }
        if(status==PASS && res != test_cases[i].exp_val)    //we only care about the result if we wanted to pass
        {
            printfcol(RED_FONT, "\nfailed test case %d (expected %d, got %d):\n%s\n\n",
                i, test_cases[i].exp_val, res, test_cases[i].src);
            exit(-1);
        }
    }

    printfcol(GREEN_FONT, "\nall tests passed!\n");
}
