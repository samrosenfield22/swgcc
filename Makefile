
hellomake:
	gcc -o swgcc.exe main.c interpreter.c lexer/lexer.c lexer/regex.c symbol.c parser/grammar.c parser/recdesc.c parser/ptree.c parser/semantic.c parser/typesys.c intermediate.c simulator.c ds/tree.c ds/vector.c lexer/nfa/nfa_build.c lexer/nfa/nfa_run.c lexer/nfa/stack/stack.c utils/misc.c utils/printcolor.c -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -ggdb
