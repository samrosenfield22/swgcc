

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "interpreter.h"

#include "lexer/lexer.h"
#include "parser/recdesc.h"
#include "parser/semantic.h"
#include "simulator.h"
#include "utils/printcolor.h"

char strbuf[801];

//statics
char *read_stmtlist(void);
int count_char_in_str(const char *str, char c);

void launch_interpreter(void)
{
	while(1)
    {
        char *in = read_stmtlist();
        if(!in)
        {
            printf("lmao\n");
            assert(0);
        }

        interpreter(NULL, VERBOSE, in);
    }
}


fail_type interpreter(int *result, bool verbose, char *code)
{
	//////////// lexer ////////////
    void *tokens = lexer(code);
    if(!tokens)
        return LEX_FAIL;
    if(verbose) lex_tokens_dump(tokens);

    //////////// parser ////////////
    void *parse_tree = parse(tokens, verbose);
    if(!parse_tree)
    {
        printfcol(RED_FONT, "parse failed\n");
        return PARSE_FAIL;
    }
    if(verbose) ptree_print(parse_tree);

    //////////// semantics ////////////
    bool declaration_only = false;
    if(!all_semantic_checks(&declaration_only, parse_tree))
    {
        printfcol(RED_FONT, "semantic fail\n");
        return SEMANTIC_FAIL;
    }
    if(verbose) ptree_print(parse_tree);

    if(verbose) printf("\n--- semacts ---\n");
    if(verbose) ptree_action(parse_tree, semact_print, true);
    if(verbose) printf("----------------\n\n");

    //////////// intermediate ////////////
    generate_intermediate_code(parse_tree);
    if(verbose) printf("intermediate code for source code: %s\n", code);
    if(verbose) dump_intermediate();

    //////////// simulator ////////////
    int res = 0;
    if(declaration_only)
    	skip_code();
    else
    	res = run_intermediate_code(verbose);
    if(verbose) dump_symbol_table();
    if(verbose) printf("\n%d\n", res);

    if(result)
        *result = res;
    return PASS;
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