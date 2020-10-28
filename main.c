

#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "symbol.h"
#include "parse.h"
#include "simulator.h"

char strbuf[801];

int count_char_in_str(const char *str, char c)
{
    int cnt = 0;
    //for(char *p=str; p; p = strchr(p, c))
    //    cnt++;
    for(const char *p=str; *p; p++)
        if(*p == c)
            cnt++;
    return cnt;
}
char *read_stmtlist(void)
{
    char inbuf[81];

    int brackets=0;
    strbuf[0] = '\0';
    
    printf(">> ");
    do
    {
        for(int i=0; i<brackets; i++) putchar('\t');

        fgets(inbuf, 80, stdin);
        inbuf[strlen(inbuf)-1] = '\0';

        //count parens and brackets
        brackets += count_char_in_str(inbuf, '{');
        brackets -= count_char_in_str(inbuf, '}');
        
        //printf("%d parens, %d brackets\n", parens, brackets);
        snprintf(strbuf, 800, "%s%s ", strbuf, inbuf);

        if(brackets < 0)
            return NULL;
        
    } while(brackets>0);

    printf("\n%s\n", strbuf);
    return strbuf;
}

int main(void)
{
    //int a;
    //for(if(2>1) {a=2;} else {a=3;};     a<10;       a++) {printf("%d\n", a);}

    nfa_builder_initialize();
    nfa_simulator_initialize();

    printf("building regexes... ");
    lexer_build_all_regexes();
    printf("done!\n");

    symbol_table_initialize();

    //char inbuf[81], strbuf[800];
    while(1)
    {
        /*parens=0; brackets=0;
        printf(">> ");
        fgets(inbuf, 80, stdin);
        inbuf[strlen(inbuf)-1] = '\0';*/

        char *in = read_stmtlist();
        if(!in)
        {
            printf("lmao\n");
            continue;
        }

        void *tokens = lexer(in);
        if(!tokens)
            continue;
        dump_symbol_table();

        void *parse_tree = parse(tokens);
        if(!parse_tree)
            continue;
        print_ptree(parse_tree);
        printf("\n\n");
        
        //int res = ptree_evaluate(parse_tree);
        generate_intermediate_code(parse_tree);
        dump_intermediate();

        int res = run_intermediate_code();
        printf("\n%d\n", res);
    }

    
    return 0;
}
