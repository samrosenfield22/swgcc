

#include <stdio.h>
#include <stdbool.h>

#include "recdesc.h"

int main(void)
{
	productions_to_parse_table();

	/*int *mpt = (int*)manual_parse_table;
	for(int i=0; i<8; i++)
	{
		printf("%s\t", nt_strings[i]);
		for(int j=0; j<9; j++)
			printf("%d\t", mpt[i*9+j]);
		putchar('\n');
	}
	printf("\n\n");
	for(int i=0; i<8; i++)
	{
		printf("%s\t", nt_strings[i]);
		for(int j=0; j<9; j++)
			printf("%d\t", parse_table[i*9+j]);
		putchar('\n');
	}

	//return 0;*/

	//lextok *dummy = chars_to_substrings_lexer("(a(b|c))+|xyz|fg?h");
	lextok *dummy = chars_to_substrings_lexer("([a-e]|_)+.txt");

	//test lexer
	for(lextok *l=dummy; l->str; l++)
		printf("%s", l->str);

	void *tree = parse(dummy);
	if(!tree)
	{
		printf("\nparser failed!\n");
		return 0;
	}
	ptree_traverse_dfs(tree, node_print, true);
	ptree_traverse_dfs(tree, semact_print, true);
	return 0;
}