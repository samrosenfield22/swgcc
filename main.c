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

void banner(void);
void test_compiler(bool verbose, int problem_case);


int main(int argc, char *argv[])
{
    bool run_tests = false;
    bool verbosity = VERBOSE;
    for(int i=1; i<argc; i++)
    {
        if(strcmp(argv[i], "-h")==0 || strcmp(argv[i], "--help")==0)
        {
            printfcol(YELLOW_FONT, "swgcc -- the swg c compiler\n");
            printf("(still in development)\n\n");

            printf("format: swgcc {options}\n");
            printf("options:\n");
            printf("-h, --help\t\tdisplay this message\n");
            printf("-t, --test\t\trun unit tests before launching interpreter\n");
            printf("-q, --quiet\t\trun interpreter without extra diagnostics (tests are quiet regardless)\n");
            printf("\nfor more information on what features of the c language are supported, consult manual.txt\n");
            return 0;
        }

        else if(strcmp(argv[i], "-t")==0 || strcmp(argv[i], "--test")==0)
            run_tests = true;
        else if(strcmp(argv[i], "-q")==0 || strcmp(argv[i], "--quiet")==0)
            verbosity = SILENT;
        else
        {
            printf("invalid option \'%s\'\n", argv[i]);
            printf("run \'%s -h\' for more details\n", argv[0]);
            return 0;
        }
    }

    banner();

    symbol_table_initialize();
    lexer_initialize();

    load_grammar("parser/c_grammar.txt");
    //dump_classnames();
    //dump_parse_table(g->parse_table);

    //run test cases
    if(run_tests)
        test_compiler(SILENT, -1);

    //interpret forever
    launch_interpreter(verbosity);

    
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
    {"int b,c=1,p,pp,n=0;", PASS, 0},
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
    //{"{while(c > 40220) {if(c & 0b1) n++; c--;} n;}", PASS, 50},

    {"int fn(void) {return 11<<1;}", PASS, 0},
    {"fn() + 5;", PASS, 27},
    {"int mulab(void) {return b * a;}", PASS, 0},
    {"int afact(void) {b=1; while(a) {b = mulab(); a--;} return b;}", PASS, 0},
    {"{a=5; afact();}", PASS, 120},

    //locals
    {"{int cc=5; cc++; a=cc;}", PASS, 0},
    {"a;", PASS, 6},

    //code that should fail
    //{"int 9a;", LEX_FAIL, 0},
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

void test_compiler(bool verbose, int problem_case)
{
    printf("\nrunning test cases..... ");

    const char *fail_strings[] = {"", "lexer", "parser", "semantic"};
    int res;
    for(int i=0; i<sizeof(test_cases)/sizeof(test_cases[0]); i++)
    {
        printfcol(GREEN_FONT, "testing case %d: %s\n", i, test_cases[i].src);
        if(i == problem_case)
            verbose = VERBOSE;

        int status = interpreter(&res, verbose, test_cases[i].src);
        if(status != test_cases[i].exp_status)
        {
            if(status == PASS)
            {
                printfcol(RED_FONT, "\nexpected failure on case %d (%s), passed instead\n",
                    i, fail_strings[test_cases[i].exp_status]);
            }
            else
            {
                printfcol(RED_FONT, "\nfailed test case %d (%s fail instead of %s):\n%s\n",
                    i, fail_strings[status], fail_strings[test_cases[i].exp_status], test_cases[i].src);
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

void banner(void)
{

const char *banner = "\n   _|_|_|  _|          _|    _|_|_|    _|_|_|    _|_|_|   \n\
 _|        _|          _|  _|        _|        _|           \n\
   _|_|    _|    _|    _|  _|  _|_|  _|        _|           \n\
       _|    _|  _|  _|    _|    _|  _|        _|           \n\
 _|_|_|        _|  _|        _|_|_|    _|_|_|    _|_|_| \n";
    puts(banner);
}
