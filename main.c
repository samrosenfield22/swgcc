

#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "symbol.h"
#include "parse.h"
#include "simulator.h"

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

    char inbuf[161];
    while(1)
    {
        printf(">> ");
        fgets(inbuf, 80, stdin);
        inbuf[strlen(inbuf)-1] = '\0';

        void *tokens = lexer(inbuf);
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
