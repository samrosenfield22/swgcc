

#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include "parser/ptree.h"	//why tho
#include "utils/printcolor.h"

#define SIM_STACK_OFFSET    (0x0000)
#define SIM_CODE_OFFSET     (0x1000)
#define SIM_VARS_OFFSET     (0x8000)    //

typedef struct intermediate_spec_s
{
    char *op;
    int arg;
} intermediate_spec;

//defined in simulator.c
extern char SIM_MEM[];
extern char *sp, *bp;
extern int eax;
extern intermediate_spec *ip, *ip_start, *ip_end;


int run_intermediate_code(bool verbose);
void skip_code(void);

int print_instr(intermediate_spec *instr);
int print_reg_or_val(int arg);
void dump_stack(void);

int define_var(symbol *sym);
char *get_var_addr(symbol *variable);

void *get_code_addr(void);
unsigned char *get_new_var(size_t bytes);

#endif //SIMULATOR_H_