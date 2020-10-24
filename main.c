

#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "symbol.h"
#include "parse.h"
#include "simulator.h"

int main(void)
{
    nfa_builder_initialize();
    nfa_simulator_initialize();

    printf("building regexes... ");
    lexer_build_all_regexes();
    printf("done!\n");

    symbol_table_initialize();

    char inbuf[161];
    while(1)
    {
        printf(">> ");
        fgets(inbuf, 80, stdin);
        inbuf[strlen(inbuf)-1] = '\0';

        void *tokens = lexer_new(inbuf);
        void *parse_tree = parse(tokens);

        if(parse_tree)
        {
            //print_ptree(parse_tree);
            //printf("\n\n");

            int res = ptree_evaluate(parse_tree);
            printf("%d\n", res);
        }
    }

    
    return 0;


    /////////////////////////////////////////////

    const char *exp = "(a|b|c)+.txt";

    nfa_model *model = regex_compile(exp);
    printf("nfa model built!\n");

    char buf[81];
    while(1)
    {
        printf("\n>> ");
        fgets(buf, 80, stdin);
        buf[strlen(buf)-1] = '\0';
        printf("%s\t\t ", buf);

        int moves = nfa_run(model, buf);
        
        if(moves != -1)
            printf("(%d moves)\n", moves);
        else
            printf("(no match)");
    }


    symbol_table_initialize();

    //char inbuf[81];
    while(1)
    {
        printf(">> ");
        fgets(inbuf, 80, stdin);
        inbuf[strlen(inbuf)-1] = '\0';

        void *tokens = lexer(inbuf);
        void *parse_tree = parse(tokens);

        if(parse_tree)
        {
            print_ptree(parse_tree);
            printf("\n\n");

            int res = ptree_evaluate(parse_tree);
            printf("%d\n", res);
        }
    }
    return 0;

    /*char inbuf[81];
    void *parse_tree;
    int res;

    while(1)
    {
        printf(">> ");
        fgets(inbuf, 80, stdin);
        parse_tree = parse(inbuf);
        res = ptree_evaluate(parse_tree);
        printf("%d\n", res);
    }

    return 0;*/
}
