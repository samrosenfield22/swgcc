
hellomake:
	gcc -o main.exe main.c lexer/lexer.c lexer/regex.c symbol.c parser/grammar.c parser/recdesc.c parser/tree.c ../swglib/automata/nfa/nfa_build.c ../swglib/automata/nfa/nfa_run.c ../swglib/structures/stack/stack.c -Wall -ggdb
