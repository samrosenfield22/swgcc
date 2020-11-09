//  gcc -o main.exe main.c regex.c ../../swglib/automata/nfa/nfa_build.c ../../swglib/automata/nfa/nfa_run.c ../../swglib/structures/stack/stack.c $(FLAGS)

#include <stdio.h>
#include <string.h>

#include "lexer/lexer.h"
#include "symbol.h"
#include "parser/recdesc.h"
#include "parser/semantic.h"
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

    //printf("\n%s\n", strbuf);
    return strbuf;
}

int main(void)
{
    symbol_table_initialize();
    lexer_initialize();
    

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
        lex_tokens_dump(tokens);
        //dump_symbol_table();
        //continue;

        void *parse_tree = parse(tokens);
        if(!parse_tree)
        {
            printf("parse failed\n");
            continue;
        }

        //ptree_traverse_dfs(parse_tree, NULL, node_print, true);
        //continue;

        if(!check_variable_declarations(parse_tree))
        {
            printf("semantic fail\n");
            continue;
        }

        //ptree_traverse_dfs(parse_tree, NULL, node_print, true);
        ptree_print(parse_tree);

        printf("\n--- semacts ---\n");
        ptree_traverse_dfs(parse_tree, NULL, semact_print, true);
        printf("----------------\n\n");

        generate_intermediate_code(parse_tree);

        int res = run_intermediate_code();
        dump_symbol_table();
        printf("\n%d\n", res);

        /*
        //int res = ptree_evaluate(parse_tree);
        generate_intermediate_code(parse_tree);
        dump_intermediate();

        int res = run_intermediate_code();
        printf("\n%d\n", res);*/
    }

    
    return 0;
}
