

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "symbol.h"

symbol **SYMBOL_TABLE;

//const char *type_idents[] = {"int", "char", "float", "double", "short", "long"};
//const char *other_keywords[] = {"if", "else", "do", "for", "while", "switch", "case"};
void symbol_table_initialize(void)
{
    /*for(int i=0; i<sizeof(type_idents)/sizeof(type_idents[0]); i++)
        symbol_create(type_idents[i], SYM_TYPE_KW);
    for(int i=0; i<sizeof(other_keywords)/sizeof(other_keywords[0]); i++)
        symbol_create(other_keywords[i], SYM_OTHER_KW);
    */

    SYMBOL_TABLE = vector(*SYMBOL_TABLE, 0);
}

symbol *symbol_search(const char *name, symbol_type type)
{
    //printf("searching for symbol \'%s\'... ", name);
    for(int i=0; i<vector_len(SYMBOL_TABLE); i++)
    {
        if((SYMBOL_TABLE[i]->type == type) || (type==SYM_ANY))
        {
            if(strcmp(name, SYMBOL_TABLE[i]->name)==0)
            {
                //printf("found at addr %p\n", &SYMBOL_TABLE[i]);
                return SYMBOL_TABLE[i];
            }
        }
    }
    //printf("not found\n");
    return NULL;
}

symbol *symbol_create(const char *name, symbol_type type)
{
    vector_append(SYMBOL_TABLE, malloc(sizeof(symbol)));

    vector_last(SYMBOL_TABLE)->name = strdup(name);
    vector_last(SYMBOL_TABLE)->type = type;
    vector_last(SYMBOL_TABLE)->var = NULL;
    vector_last(SYMBOL_TABLE)->declared = false;
    vector_last(SYMBOL_TABLE)->initialized = false;

    return vector_last(SYMBOL_TABLE);
}

void dump_symbol_table(void)
{
    printf("symbol table\n------------------\n");
    for(int i=0; i<vector_len(SYMBOL_TABLE); i++)
    {
        printf("symbol %d (%d):\t\'%s\'\t", i, (int)SYMBOL_TABLE[i]->var, SYMBOL_TABLE[i]->name);
        if(SYMBOL_TABLE[i]->type==SYM_TYPE_KW)
            printf("type");
        else if(SYMBOL_TABLE[i]->type==SYM_OTHER_KW)
            printf("keyword");
        else if(!SYMBOL_TABLE[i]->declared)
            printf("undeclared variable");
        //else if(!SYMBOL_TABLE[i].initialized)
        //    printf("uninitialized variable");
        else
            printf(" = %d", *(SYMBOL_TABLE[i]->var));

        printf("\n");
    }

    printf("\n\n\n");
}
