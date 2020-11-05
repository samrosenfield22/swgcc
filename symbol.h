

#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <stdbool.h>

#include "ds/vector.h"

typedef enum symbol_type_e
{
    SYM_ANY,
    SYM_IDENTIFIER,
    SYM_TYPE_KW,
    SYM_OTHER_KW
} symbol_type;

typedef struct symbol_s
{
    symbol_type type;
    char *name;
    int *var;
    
    bool declared;
    bool initialized;
} symbol;

void symbol_table_initialize(void);
symbol *symbol_search(const char *name, symbol_type type);
symbol *symbol_create(const char *name, symbol_type type);
void dump_symbol_table(void);

#endif //SYMBOL_H_
