

#ifndef SEMANTIC_H_
#define SEMANTIC_H_

#include "tree.h"
#include "../simulator.h"
#include "../utils/printcolor.h"

bool all_semantic_checks(bool *decl_only, node *pt);

bool check_variable_declarations(node *pt);
bool handle_lvals(node *pt);
bool set_conditional_jumps(node *pt);

#endif //SEMANTIC_H_