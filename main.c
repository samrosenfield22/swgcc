//  gcc -o main.exe main.c regex.c ../../swglib/automata/nfa/nfa_build.c ../../swglib/automata/nfa/nfa_run.c ../../swglib/structures/stack/stack.c $(FLAGS)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>

#include "lexer/lexer.h"
#include "parser/recdesc.h"
#include "interpreter.h"
#include "symbol.h"
#include "parser/typesys.h"
#include "utils/printcolor.h"

_Static_assert(sizeof(long) == sizeof(int*),
    "size of long and int* need to be the same, or the simulator won't work!");

void handle_cmdline_options(int argc, char *argv[]);
void banner(void);
void test_compiler(bool verbose, int problem_case);
void close_outfile(void);

//environment variables that are set by cmdline options
bool run_tests = false;
bool verbosity = VERBOSE;
FILE **diag_printfile = &outfile;

void print_the_string(void *t)
{
    char *str = ((node*)t)->str;
    puts(str);
}

int main(int argc, char *argv[])
{
    
    handle_cmdline_options(argc, argv);
    banner();

    symbol_table_initialize();
    init_type_system();
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

    //conditionals
    {"if(a-7) 22;", PASS, 22},
    {"if(a-8) 22;", PASS, 0},
    {"{while(a) {c*=a; a--;} c;}", PASS, 40320},
    //do while {";", PASS, },
    //{"{while(c > 40220) {if(c & 0b1) n++; c--;} n;}", PASS, 50},

    //functions
    {"int fn(void) {return 11<<1;}", PASS, 0},
    {"fn() + 5;", PASS, 27},
    {"int mulab(void) {return b * a;}", PASS, 0},
    {"int afact(void) {b=1; while(a) {b = mulab(); a--;} return b;}", PASS, 0},
    {"{a=5; afact();}", PASS, 120},

    //locals
    {"{int cc=5; cc++; a=cc;}", PASS, 0},
    {"a;", PASS, 6},
    {"{int a=1; {int a=2; {int a=3;} b=a;} b;}", PASS, 0},
    {"b;", PASS, 2},
    {"int locfunc(void) {int loc1=5, loc2=6; int locres = loc1+loc2; return locres;}", PASS, 0},
    {"locfunc();", PASS, 11},

    //args
    {"int power(int base, int exp)  \
    {                               \
        int n=1;                    \
        while(exp) {                \
            n *= base;              \
            exp--;                  \
        }                           \
        return n;                   \
    }", PASS, 0},
    {"power(3,4);", PASS, 81},

    //code that should fail
    //{"int 9a;", LEX_FAIL, 0},
    //{"a = !!!;", LEX_FAIL, 0},
    {"a + -;", PARSE_FAIL, 0},
    {"int int;", PARSE_FAIL, 0},
    {"a = ((5);", PARSE_FAIL, 0},
    {"= a + 1;", PARSE_FAIL, 0},
    {"int p;", SEMANTIC_FAIL, 0},
    {"a = notdecld;", SEMANTIC_FAIL, 0},
    {"5 = a;", SEMANTIC_FAIL, 0},
    {"power(1);", SEMANTIC_FAIL, 0},
    {"fn(1);", SEMANTIC_FAIL, 0}
    //{";", , 0},
    
};

void test_compiler(bool verbose, int problem_case)
{
    printf("\nrunning test cases.....\n");

    const char *fail_strings[] =
    {
        [PASS] = "pass",
        [LEX_FAIL] = "lexer",
        [PARSE_FAIL] = "parser",
        [SEMANTIC_FAIL] = "semantic"
    };

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
                exit(-1);
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

//https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
void handle_cmdline_options(int argc, char *argv[])
{
    FILE *fp;

    while (1)
    {
        static struct option long_options[] =
        {
            /* 
            //options to set a flag (int)
            {"verbose", no_argument,       &verbose_flag, 1},
            {"brief",   no_argument,       &verbose_flag, 0},
            */

            {"help",      no_argument,    0,  'h'},
            {"test",      no_argument,    0,  't'},
            {"quiet",     no_argument,    0,  'q'},

            //options with a required arg
            {"output",  required_argument, 0, 'o'},
            {0, 0, 0, 0}
        };

        const char *explanations[] =
        {
            "display this message",                                                     //--help
            "run unit tests before launching interpreter",                              //--test
            "run interpreter without extra diagnostics (tests are quiet regardless)",    //--quiet 
            "outputs diagnostics to the given file" 
        };
      
        /* getopt_long stores the option index here. */
        int option_index = 0;

        //for options which require an arg, put a colon after
        //ex. getopt_long(argc, argv, "abc:d:f:, ...");
        int c = getopt_long (argc, argv, "tho:q", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch(c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;

            case 't':
                run_tests = true;
                break;

            case 'q':
                verbosity = SILENT;
                break;

            case 'o':
                //printf("outputting to file %s\n", optarg);
                //exit(-1);
                fp = fopen(optarg, "w");
                if(!fp)
                {
                    printf("error: failed to open output file %s\n", optarg);
                    exit(-1);
                }
                *diag_printfile = fp;
                atexit(close_outfile);
                break;

            case 'h':
                printfcol(YELLOW_FONT, "swgcc -- the swg c compiler\n");
                printf("(still in development)\n\n");

                printf("format: swgcc {options}\n");
                printf("options:\n");
                for(int i=0; i<sizeof(explanations)/sizeof(explanations[0]); i++)
                {
                    bool has_arg = long_options[i].has_arg == required_argument;
                    const char *argh = has_arg? " {arg}" : "";
                    
                    int spaces = printf("-%c%s, --%s%s",
                        long_options[i].val, argh, long_options[i].name, argh);
                    for(int i=spaces; i<32; i++) putchar(' ');
                    puts(explanations[i]);
                }
                printf("\nfor more information on what features of the c language are supported, consult manual.txt\n");
                exit(0);
                break;

            case '?':
                /* getopt_long already printed an error message. */
                printf("run \'swgcc -h\' for more info\n");
                exit(-1);
                break;

            default:
                abort();
        }

    }
}

void banner(void)
{
    const char *banner = "\n       _|_|_|  _|          _|    _|_|_|    _|_|_|    _|_|_|   \n\
     _|        _|          _|  _|        _|        _|           \n\
       _|_|    _|    _|    _|  _|  _|_|  _|        _|           \n\
           _|    _|  _|  _|    _|    _|  _|        _|           \n\
     _|_|_|        _|  _|        _|_|_|    _|_|_|    _|_|_| \n";
    puts(banner);
}

void close_outfile(void)
{
    fclose(*diag_printfile);
}
