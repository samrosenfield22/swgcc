
hellomake:
	gcc -o main.exe main.c interpreter.c lexer/lexer.c lexer/regex.c symbol.c parser/grammar.c parser/recdesc.c parser/tree.c parser/semantic.c simulator.c ds/vector.c lexer/nfa/nfa_build.c lexer/nfa/nfa_run.c lexer/nfa/stack/stack.c -Wall -ggdb
