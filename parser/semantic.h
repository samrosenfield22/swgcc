

#ifndef SEMANTIC_H_
#define SEMANTIC_H_

#include "ptree.h"
#include "../simulator.h"
#include "../utils/printcolor.h"
#include "../utils/misc.h"

bool all_semantic_checks(bool *decl_only, pnode *pt);

bool check_variable_declarations(pnode *pt);
bool handle_lvals(pnode *pt);
bool set_conditional_jumps(pnode *pt);

void function_dump_info(pnode *fid);

#endif //SEMANTIC_H_