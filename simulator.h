

#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include "parser/tree.h"

void generate_intermediate_code(node *n);
int run_intermediate_code(void);

void define_var(symbol *sym);
unsigned char *get_new_var(size_t bytes);

#endif //SIMULATOR_H_