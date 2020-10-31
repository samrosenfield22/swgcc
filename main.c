//  gcc -o main.exe main.c regex.c ../../swglib/automata/nfa/nfa_build.c ../../swglib/automata/nfa/nfa_run.c ../../swglib/structures/stack/stack.c $(FLAGS)

#include <stdio.h>
#include <string.h>

#include "lexer/lexer.h"
#include "symbol.h"
#include "parser/recdesc.h"
//#include "simulator.h"

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
    
    lexer_initialize();

    //symbol_table_initialize();

    grammar *g = load_grammar("parser/c_grammar.txt");
    dump_classnames();
    //dump_productions(g);
    dump_parse_table(g->parse_table);

    while(1)
    {

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
        {
            printf("parse failed\n");
            continue;
        }
        //print_ptree(parse_tree);
        ptree_traverse_dfs(parse_tree, node_print, true);
        ptree_traverse_dfs(parse_tree, semact_print, true);
        printf("\n\n");
        /*
        //int res = ptree_evaluate(parse_tree);
        generate_intermediate_code(parse_tree);
        dump_intermediate();

        int res = run_intermediate_code();
        printf("\n%d\n", res);*/
    }

    
    return 0;
}
