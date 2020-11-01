FLAGS=-Wall -ggdb
#FLAGS=-Wall -ggdb -pedantic

hellomake: main.c regex.c
	gcc -o main.exe main.c regex.c ../../swglib/automata/nfa/nfa_build.c ../../swglib/automata/nfa/nfa_run.c ../../swglib/structures/stack/stack.c $(FLAGS)