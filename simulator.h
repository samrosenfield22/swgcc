

#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include "parser/tree.h"
#include "utils/printcolor.h"

void generate_intermediate_code(node *n);
void dump_intermediate(void);
int run_intermediate_code(bool verbose);
void skip_code(void);

int define_var(symbol *sym);
void *get_code_addr(void);
unsigned char *get_new_var(size_t bytes);

#endif //SIMULATOR_H_