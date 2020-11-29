

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

//searches for symbols w matching type (unless passed SYM_ANY) and name
//if there are multiple (i.e. variables that shadow each other), return the deepest one
symbol *symbol_search(const char *name, symbol_type sym_type)
{

    symbol **match = vector_copy_filter(SYMBOL_TABLE, 
        (n->sym_type == sym_type || sym_type==SYM_ANY) && strcmp(name, n->name)==0);
    //symbol *m = vector_is_empty(match)? NULL : match[0];
    symbol *m = vector_is_empty(match)? NULL : vector_last(match);
    vector_destroy(match);
    return m;
}

//takes a vector of blocks (scopes). for each one, looks for a symbol (of matching name/type) in that scope.
symbol *symbol_search_local(const char *name, symbol_type sym_type, void **block)
{

    /*symbol **match = vector_copy_filter(SYMBOL_TABLE, 
        (n->sym_type == sym_type || sym_type==SYM_ANY) && strcmp(name, n->name)==0 && n->block==block);
    symbol *m = vector_is_empty(match)? NULL : match[0];*/

    symbol **match = vector_copy_filter(SYMBOL_TABLE, 
        (n->sym_type == sym_type || sym_type==SYM_ANY) && strcmp(name, n->name)==0);
    if(vector_is_empty(match))
    {
        vector_destroy(match);
        return NULL;
    }

    symbol *local = NULL;
    vector_foreach(block, b)
    {
        vector_foreach(match, m)
        {
            if(block[b] == match[m]->block)
            {
                local = match[m];
                goto local_found;
            }
        }
    }
    local_found:
    vector_destroy(match);
    return local;
}

//returns a ptr to a symbol whose var addr matches, or if there is none, a symbol (of type function)
//for whom the given varaddr is ahead of the symbol's (it's a memory address in the function)
symbol *symbol_search_by_addr(char *varaddr)
{
    symbol **match = vector_copy_filter(SYMBOL_TABLE, n->sym_type == SYM_IDENTIFIER && n->var == varaddr);
    symbol *m = vector_is_empty(match)? NULL : match[0];
    vector_destroy(match);

    return m;
}

symbol *symbol_search_function_addr(char *varaddr)
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
    vector_last(SYMBOL_TABLE)->lifetime = STATIC;
    vector_last(SYMBOL_TABLE)->scope = INTERNAL;
    vector_last(SYMBOL_TABLE)->block = NULL;
    vector_last(SYMBOL_TABLE)->is_argument = false;
    vector_last(SYMBOL_TABLE)->argct = 0;
    vector_last(SYMBOL_TABLE)->argbytes = 0;

    //if we're defining a new type, add the type specification
    if(type)
    {
        vector_last(SYMBOL_TABLE)->tspec = malloc(sizeof(typespec));
        memcpy(vector_last(SYMBOL_TABLE)->tspec, type, sizeof(typespec));
        //vector_last(SYMBOL_TABLE)->type = type;
    }

    return vector_last(SYMBOL_TABLE);
}

//bool symbol_delete(const char *name)
bool symbol_delete(symbol *sym)
{
    /*assert(0);
    symbol *sym = symbol_search(name, SYM_IDENTIFIER);
    if(!sym)
        return false;
    */
    //kinda dumb that we have to do this
    vector_foreach(SYMBOL_TABLE, i)
    {
        if(SYMBOL_TABLE[i] == sym)
        {
            vector_delete(&SYMBOL_TABLE, i);
            return true;
        }
    }
    
    return false;
}

/*void delete_locals_in_block(void *block)
{
    vector_foreach(SYMBOL_TABLE, i)
    {
        symbol *sym = SYMBOL_TABLE[i];
        if(sym->sym_type==SYM_IDENTIFIER && strcmp(sym->type->name, "function")!=0 && sym->block==block)
        {
            vector_delete(&SYMBOL_TABLE, i);
            i--;
        }
    }
}*/

void delete_all_locals(void)
{
    vector_foreach(SYMBOL_TABLE, i)
    {
        symbol *sym = SYMBOL_TABLE[i];
        //if(sym->sym_type==SYM_IDENTIFIER && strcmp(sym->type->name, "function")!=0 && sym->lifetime==AUTO)
        if(sym->sym_type==SYM_IDENTIFIER && strcmp(sym->type->name, "function")!=0 && sym->scope==BLOCK)
        {
            vector_delete(&SYMBOL_TABLE, i);
            i--;
        }
    }
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
            //printf("symbol %d:\t\'%s\' (%s)(@ %d) = %d\n",
            //    i, sym->name, sym->type->name, (int)(sym->var), *(sym->var));


            printf("symbol %d:\t\'%s\' (", i, sym->name);
            if(strcmp(sym->type->name, "function") == 0)
            {
                printf("%s, %d args)(@ %d)\n", sym->type->name, sym->argct, (int)(sym->var));
            }
            else
            {
                if(sym->lifetime==STATIC)
                    printf("static %s)(@ %d) = %d\n", sym->type->name, (int)(sym->var), *(sym->var));
                else
                    printf("auto %s)(@ bp+%d)\n", sym->type->name, (int)(sym->var));
            }   
        }
    }

    printf("\n");
}

void dump_symbol_table_oneline(void)
{
    return;
    for(int i=0; i<vector_len(SYMBOL_TABLE); i++)
    {
        if(SYMBOL_TABLE[i]->sym_type == SYM_IDENTIFIER && strcmp(SYMBOL_TABLE[i]->type->name, "function")!=0)
            printf("%s=%d   ", SYMBOL_TABLE[i]->name, *SYMBOL_TABLE[i]->var);
            //printf("%s=%d   ", SYMBOL_TABLE[i]->name, *get_var_addr(SYMBOL_TABLE[i]));

    }
}
