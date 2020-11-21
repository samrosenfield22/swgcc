FLAGS=-Wall -ggdb
#FLAGS=-Wall -ggdb -pedantic

hellomake: main.c nfa_build.c nfa_run.c
	gcc -o main.exe main.c nfa_build.c nfa_run.c ../../structures/stack/stack.c $(FLAGS)