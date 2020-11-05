

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "symbol.h"

#define SYMTABLE_LEN (400)
symbol SYMBOL_TABLE[SYMTABLE_LEN];
int sym_index = 0;

const char *type_idents[] = {"int", "char", "float", "double", "short", "long"};
const char *other_keywords[] = {"if", "else", "do", "for", "while", "switch", "case"};
void symbol_table_initialize(void)
{
    for(int i=0; i<sizeof(type_idents)/sizeof(type_idents[0]); i++)
        symbol_create(type_idents[i], SYM_TYPE_KW);
    for(int i=0; i<sizeof(other_keywords)/sizeof(other_keywords[0]); i++)
        symbol_create(other_keywords[i], SYM_OTHER_KW);
}

symbol *symbol_search(const char *name, symbol_type type)
{
    //printf("searching for symbol \'%s\'... ", name);
    for(int i=0; i<sym_index; i++)
    {
        if((SYMBOL_TABLE[i].type == type) || (type==SYM_ANY))
        {
            if(strcmp(name, SYMBOL_TABLE[i].name)==0)
            {
                //printf("found at addr %p\n", &SYMBOL_TABLE[i]);
                return &SYMBOL_TABLE[i];
            }
        }
    }
    //printf("not found\n");
    return NULL;
}

symbol *symbol_create(const char *name, symbol_type type)
{
    /*SYMBOL_TABLE[sym_index].name = malloc(strlen(name)+1);
    assert(SYMBOL_TABLE[sym_index].name);
    strcpy(SYMBOL_TABLE[sym_index].name, name);*/
    SYMBOL_TABLE[sym_index].name = strdup(name);

    SYMBOL_TABLE[sym_index].type = type;
    //SYMBOL_TABLE[sym_index].val = 0;
    SYMBOL_TABLE[sym_index].var = NULL;
    SYMBOL_TABLE[sym_index].declared = false;
    SYMBOL_TABLE[sym_index].initialized = false;

    symbol *symaddr = &SYMBOL_TABLE[sym_index];
    sym_index++;
    return symaddr;
}

void dump_symbol_table(void)
{
    printf("symbol table\n------------------\n");
    for(int i=0; i<sym_index; i++)
    {
        printf("symbol %d (%p):\t\'%s\'\t", i, &SYMBOL_TABLE[i], SYMBOL_TABLE[i].name);
        if(SYMBOL_TABLE[i].type==SYM_TYPE_KW)
            printf("type");
        else if(SYMBOL_TABLE[i].type==SYM_OTHER_KW)
            printf("keyword");
        else if(!SYMBOL_TABLE[i].declared)
            printf("undeclared variable");
        //else if(!SYMBOL_TABLE[i].initialized)
        //    printf("uninitialized variable");
        else
            printf(" = %d", *(SYMBOL_TABLE[i].var));

        printf("\n");
    }

    printf("\n\n\n");
}
