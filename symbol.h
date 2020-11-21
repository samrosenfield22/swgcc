

#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <stdbool.h>

#include "ds/vector.h"

typedef enum symbol_type_e
{
    SYM_ANY,
    SYM_IDENTIFIER,
    //SYM_FUNCTION,
    SYM_TYPESPEC,
    SYM_OTHER_KW
} symbol_type;

typedef struct typespec_s
{
    size_t bytes;
} typespec;

typedef struct symbol_s symbol;

struct symbol_s
{
    symbol_type sym_type;
    char *name;
    int *var;

    //scope
    //type
    symbol *type;

    //only for types
    typespec *tspec;
    
    bool declared;
    bool initialized;
};

void symbol_table_initialize(void);
symbol *symbol_search(const char *name, symbol_type sym_type);
symbol *symbol_create(const char *name, symbol_type sym_type, typespec *type);
bool symbol_delete(const char *name);
void assign_type_to_symbol(symbol *sym, const char *typestr);

void dump_symbol_table(void);
void dump_symbol_table_oneline(void);

#endif //SYMBOL_H_
