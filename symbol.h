

#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <stdbool.h>
//#include "simulator.h"

#include "ds/tree.h"

#include "ds/vector.h"
#include "utils/printcolor.h"

#include "utils/misc.h"

#define SIM_INT_SIZE    sizeof(long)

typedef enum symbol_type_e
{
    SYM_ANY,
    SYM_IDENTIFIER,
    SYM_BASIC_TYPE,
    //SYM_OTHER_KW
} symbol_type;

typedef struct typespec_s
{
    size_t bytes;
} typespec;

typedef struct symbol_s symbol;

#define STATIC  true
#define AUTO    false

typedef enum scopetype_e
{
    BLOCK,
    INTERNAL,
    EXTERNAL
} scopetype;

struct symbol_s
{
    struct  //must be first (symbols are dag nodes)
    {
        void **parents;
        void **children;
        char *str;
    };

    //bool sym_or_type;

    symbol_type sym_type;
    char *name;
    long *var;

    //scope
    //lifetime
    bool lifetime;
    scopetype scope;
    void *block;    //if the var is block scope, this points to the containing block
    bool is_argument;

    //for function symbols
    //when the type sys is working we won't need these
    size_t argct;
    size_t argbytes;    //number of argument bytes to get cleaned up by caller

    //type
    symbol *type;
    //void *type;

    //only for types
    //typespec *tspec;
    size_t type_bytes;
    
    bool declared;
    bool initialized;
};

void symbol_table_initialize(void);
symbol *symbol_search(const char *name, symbol_type sym_type);
symbol *symbol_search_local(const char *name, symbol_type sym_type, void **block);
symbol *symbol_search_by_addr(char *varaddr);
symbol *symbol_search_function_addr(char *varaddr);
//symbol *symbol_create(const char *name, symbol_type sym_type, typespec *type);
symbol *symbol_create(const char *name, symbol_type sym_type, size_t type_bytes);
//bool symbol_delete(const char *name);
bool symbol_delete(symbol *sym);
void delete_locals_in_block(void *block);
void delete_all_locals(void);
void assign_type_to_symbol(symbol *sym, const char *typestr);

void dump_symbol_table(void);
void dump_symbol_table_oneline(void);

#endif //SYMBOL_H_
