

#include <stdio.h>
#include <string.h>

#include "regex.h"

nfa_model *regex_compile(const char *regex)
{
	void *parse_tree = parse(regex);
    //print_ptree(parse_tree);

    //printf("\n\n\n\n");
    //ptree_extract_dfs(parse_tree);
    nfa_model *n = ptree_to_nfa(parse_tree);
    printf("\n%p\n", n);
    nfa_dump(n);

    return n;
}

int main(void)
{
    //const char *exp = "   (a|b)* | xy?z";
    //const char *exp = "ab";
    //const char *exp = "[a-zA-Z]+.txt";
    const char *exp = "(a|b|c)*txt";
    //const char *exp = "\\+ | - | \\* | \\/ | \\% | \\&\\& | \\|\\| | \\^ | \\| | \\&";

    nfa_builder_initialize();
    nfa_simulator_initialize();

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
    

    return 0;
}
