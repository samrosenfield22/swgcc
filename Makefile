
hellomake:
	gcc -o main.exe main.c lexer.c symbol.c parse.c simulator.c regex/regex.c ../swglib/automata/nfa/nfa_build.c ../swglib/automata/nfa/nfa_run.c ../swglib/structures/stack/stack.c -Wall -ggdb
