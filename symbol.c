

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

    symbol_create("int", SYM_TYPESPEC, &(typespec){4});
    symbol_create("char", SYM_TYPESPEC, &(typespec){1});
    symbol_create("short", SYM_TYPESPEC, &(typespec){2});
    symbol_create("function", SYM_TYPESPEC, &(typespec){0});

    //symbol_create("lmao", SYM_IDENTIFIER, NULL);

    //printf("symbol 0 is %s (%d bytes)\n", SYMBOL_TABLE[0]->name, SYMBOL_TABLE[0]->type->bytes);
    //exit(0);
}

symbol *symbol_search(const char *name, symbol_type sym_type)
{

    symbol **match = vector_copy_filter(SYMBOL_TABLE, 
        (n->sym_type == sym_type || sym_type==SYM_ANY) && strcmp(name, n->name)==0);
    symbol *m = vector_is_empty(match)? NULL : match[0];
    vector_destroy(match);
    return m;
    //////////////////////////////////////////

    //printf("searching for symbol \'%s\'... ", name);
    /*for(int i=0; i<vector_len(SYMBOL_TABLE); i++)
    {
        if((SYMBOL_TABLE[i]->sym_type == sym_type) || (sym_type==SYM_ANY))
        {
            if(strcmp(name, SYMBOL_TABLE[i]->name)==0)
            {
                //if(sym_type == SYM_TYPESPEC)
                //    printf("sym %d: found type (%s), size = %d\n", i, name, SYMBOL_TABLE[i]->tspec->bytes);

                return SYMBOL_TABLE[i];
            }
        }
    }
    //printf("not found\n");
    return NULL;*/
}

//returns a ptr to a symbol whose var addr matches, or if there is none, a symbol (of type function)
//for whom the given varaddr is ahead of the symbol's (it's a memory address in the function)
symbol *symbol_search_by_addr(int *varaddr)
{
    symbol **match = vector_copy_filter(SYMBOL_TABLE, n->sym_type == SYM_IDENTIFIER && n->var == varaddr);
    symbol *m = vector_is_empty(match)? NULL : match[0];
    vector_destroy(match);

    return m;
}

symbol *symbol_search_function_addr(int *varaddr)
{
    symbol **match = vector_copy_filter(SYMBOL_TABLE,
        n->sym_type == SYM_IDENTIFIER && strcmp(n->type->name, "function")==0 && n->var < varaddr);
    symbol *m = vector_is_empty(match)? NULL : vector_last(match);  //assumes that functions are stored in the symbol
    vector_destroy(match);

    return m;
}

symbol *symbol_create(const char *name, symbol_type sym_type, typespec *type)
{
    assert(sym_type != SYM_ANY);
    if(sym_type == SYM_ANY)
        return NULL;

    vector_append(SYMBOL_TABLE, malloc(sizeof(symbol)));

    vector_last(SYMBOL_TABLE)->name = strdup(name);
    vector_last(SYMBOL_TABLE)->sym_type = sym_type;
    vector_last(SYMBOL_TABLE)->var = NULL;
    vector_last(SYMBOL_TABLE)->declared = false;
    vector_last(SYMBOL_TABLE)->initialized = false;

    //if we're defining a new type, add the type specification
    if(type)
    {
        vector_last(SYMBOL_TABLE)->tspec = malloc(sizeof(typespec));
        memcpy(vector_last(SYMBOL_TABLE)->tspec, type, sizeof(typespec));
        //vector_last(SYMBOL_TABLE)->type = type;
    }

    return vector_last(SYMBOL_TABLE);
}

bool symbol_delete(const char *name)
{
    symbol *sym = symbol_search(name, SYM_IDENTIFIER);
    if(!sym)
        return false;

    //kinda dumb that we have to do this
    vector_foreach(SYMBOL_TABLE, i)
    {
        if(SYMBOL_TABLE[i] == sym)
        {
            vector_delete(&SYMBOL_TABLE, i);
            break;
        }
    }
    
    return true;
}

void assign_type_to_symbol(symbol *sym, const char *typestr)
{
    symbol *typesym = symbol_search(typestr, SYM_TYPESPEC);
    if(!typesym)
    {
        assert(0);
        return;
    }
    sym->type = typesym;
}

void dump_symbol_table(void)
{
    printf("\nsymbol table\n------------------\n");
    for(int i=0; i<vector_len(SYMBOL_TABLE); i++)
    {
        if(SYMBOL_TABLE[i]->sym_type==SYM_IDENTIFIER)
        {
            symbol *sym = SYMBOL_TABLE[i];
            printf("symbol %d:\t\'%s\' (%s)(@ %d) = %d\n",
                i, sym->name, sym->type->name, (int)(sym->var), *(sym->var));
        }
        /*else if(SYMBOL_TABLE[i]->sym_type==SYM_FUNCTION)
        {
            symbol *sym = SYMBOL_TABLE[i];
            printf("symbol %d:\t\'%s\' (%s)(@ %d) = %d\n",
                i, sym->name, sym->type->name, (int)(sym->var), *(sym->var));
        }*/
        /*
        //if(SYMBOL_TABLE[i]->sym_type==SYM_IDENTIFIER)
        printf("symbol %d:\t\'%s\'", i, SYMBOL_TABLE[i]->name);

        if(SYMBOL_TABLE[i]->sym_type==SYM_IDENTIFIER)
            printf(" (%s)(@ %d) = %d",
                SYMBOL_TABLE[i]->type->name, (int)(SYMBOL_TABLE[i]->var), *(SYMBOL_TABLE[i]->var));
        else if(SYMBOL_TABLE[i]->sym_type==SYM_TYPESPEC)
            printf("\ttype, %d bytes", SYMBOL_TABLE[i]->tspec->bytes);
        //else if(SYMBOL_TABLE[i]->sym_type==SYM_OTHER_KW)
        //    printf("keyword");
        else if(!SYMBOL_TABLE[i]->declared)
            printf("undeclared variable");
        //else if(!SYMBOL_TABLE[i].initialized)
        //    printf("uninitialized variable");

        printf("\n");
        */
    }

    printf("\n\n\n");
}

void dump_symbol_table_oneline(void)
{
    //return;
    for(int i=0; i<vector_len(SYMBOL_TABLE); i++)
    {
        if(SYMBOL_TABLE[i]->sym_type == SYM_IDENTIFIER && strcmp(SYMBOL_TABLE[i]->type->name, "function")!=0)
            printf("%s=%d   ", SYMBOL_TABLE[i]->name, *SYMBOL_TABLE[i]->var);

    }
}
