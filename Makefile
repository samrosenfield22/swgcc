
hellomake:
	gcc -o main.exe main.c interpreter.c lexer/lexer.c lexer/regex.c symbol.c parser/grammar.c parser/recdesc.c parser/tree.c parser/semantic.c simulator.c ds/vector.c ../swglib/automata/nfa/nfa_build.c ../swglib/automata/nfa/nfa_run.c ../swglib/structures/stack/stack.c -Wall -ggdb
