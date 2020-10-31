

#include <stdio.h>
#include <stdbool.h>

#include "grammar.h"
#include "recdesc.h"

int main(void)
{
		grammar *g = load_grammar("regex_grammar.txt");
		dump_classnames();
		//dump_productions(g);
		dump_parse_table(g->parse_table);
		//return 0;

		lextok *dummy = chars_to_substrings_lexer("(a(b|c))+|xyz|fg?h");
		//lextok *dummy = chars_to_substrings_lexer("abc");
		//lextok *dummy = chars_to_substrings_lexer("a?b");

		//test lexer
		for(lextok *l=dummy; l->str; l++)
			printf("%s", l->str);
		printf("\n\n");

		void *tree = parse(dummy);
		if(!tree)
		{
			printf("\nparser failed!\n");
			return 0;
		}

		ptree_init_names(g->nonterminals);
		ptree_traverse_dfs(tree, node_print, true);
		ptree_traverse_dfs(tree, semact_print, true);
		return 0;
}
