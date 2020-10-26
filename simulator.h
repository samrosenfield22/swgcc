

#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include "parse.h"
#include "symbol.h"

void generate_intermediate_code(node *n);
int run_intermediate_code(void);
void dump_intermediate(void);

//int ptree_evaluate(node *n);


#endif //SIMULATOR_H_
