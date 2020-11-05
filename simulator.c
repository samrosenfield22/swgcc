

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "simulator.h"

static bool filter_semact(node *n);
static void generate_instruction(node *n, int depth);

void sim_stack_push(int n);
int sim_stack_pop(void);

int add_op(void);
int sub_op(void);
int mult_op(void);
int div_op(void);
int mod_op(void);
int shl_op(void);
int shr_op(void);
int leq_op(void);
int geq_op(void);
int eq_op(void);
int neq_op(void);
int bw_and_op(void);
int bw_or_op(void);
int bw_xor_op(void);
int log_and_op(void);
int log_or_op(void);
int comma_op(void);
int assign_op(void);

typedef struct intermediate_spec_s
{
    //ptree_tok_type type;
    char *op;
    int arg;
} intermediate_spec;
intermediate_spec code[1000];
int ispec_index = 0;

unsigned char SIM_MEM[0x10000];
#define SIM_STACK_OFFSET    (0x0000)
#define SIM_VARS_OFFSET     (0x8000)    //

unsigned char *var_addr = SIM_MEM + SIM_VARS_OFFSET;

int sim_stack[256];
int *sp = sim_stack;
int ip;

void generate_intermediate_code(node *n)
{
    ispec_index = 0;

    ptree_traverse_dfs(n, filter_semact, generate_instruction, true);

    //generate_intermediate_code_recursive(n);
    //dump_intermediate();
    //resolve_jump_addresses();
    //return code;
}

unsigned char *get_new_var(size_t bytes) //or a ptr to a type (in a type table?)
{
    unsigned char *addr = var_addr;
    var_addr += bytes;
    return addr;
}

static bool filter_semact(node *n)
{
	return (!(n->is_nonterminal) && n->type == SEMACT);
}

static void generate_instruction(node *n, int depth)
{
	printf("generating instruction from semact: %s\n", n->str);

    char *subs = strtok(n->str, " ");

	//code[ispec_index].op = malloc(strlen(subs)+1);
    //strcpy(code[ispec_index].op, subs);
    code[ispec_index].op = strdup(subs);

    subs = strtok(NULL, " ");
    if(subs)
        code[ispec_index].arg = strtol(subs, NULL, 10);

    printf("instruction %d\n\top: %s\n\targ: %d\n\n", ispec_index, code[ispec_index].op, code[ispec_index].arg);

	ispec_index++;
}

struct op_entry
{
    char *op;
    int (*func)(void);
} op_table[] =
{
    {"+",       add_op},
    {"-",    sub_op},
    {"*",    mult_op},
    {"/",    div_op},
    {"%",    mod_op},
    {"<<",    shl_op},
    {">>",    shr_op},
    {"=",    assign_op}
    //{"",    _op},

};

int run_intermediate_code(void)
{
    sp = sim_stack;
    ip = 0;

    //for(int i=0; i<ispec_index; i++)
    for(ip=0; ip<ispec_index; ip++)
    {
        intermediate_spec *instr = &code[ip];

        if(strcmp(instr->op, "push")==0)
            sim_stack_push(instr->arg);
        else if(strcmp(instr->op, "pop")==0)
            sim_stack_pop();    //what do with result?
        else
        {
            for(int i=0; i<sizeof(op_table)/sizeof(op_table[0]); i++)
            {
                if(strcmp(instr->op, op_table[i].op)==0)
                {
                    op_table[i].func();
                    break;
                }
            }
        }
    }

    return sim_stack_pop();
}

void sim_stack_push(int n)
{
    *sp++ = n;
}

int sim_stack_pop(void)
{
    --sp;
    return *sp;
}

//all binary operators follow the same semantic action format
#define def_bin_op(name,op)     \
  int name##_op(void)      \
  {                             \
      int b = sim_stack_pop();  \
      int a = sim_stack_pop();  \
      sim_stack_push(a op b);   \
      return 0;                 \
  }

def_bin_op(add,+)
def_bin_op(sub,-)
def_bin_op(mult,*)
def_bin_op(div,/)
def_bin_op(mod,%)
def_bin_op(shl,<<)
def_bin_op(shr,>>)
def_bin_op(leq,<=)
def_bin_op(geq,>=)
def_bin_op(eq,==)
def_bin_op(neq,!=)
def_bin_op(bw_or,|)
def_bin_op(bw_and,&)
def_bin_op(bw_xor,^)
def_bin_op(log_or,||)
def_bin_op(log_and,&&)
//def_bin_op(comma, ,)
//def_bin_op(,)

int comma_op(void)
{
    int b = sim_stack_pop();
    sim_stack_pop();    //throw away value
    sim_stack_push(b);
    return 0;
}

int assign_op(void)
{
    int b = sim_stack_pop();
    int *lv = (int*)sim_stack_pop();
    //printf("assigning value %d to variable\n", b);

    //symbol *as = (symbol *)sim_stack_pop();
    //int *val = as->val;

    //printf("simulator: var w symbol at addr %p, value is %d\n", as, as->val);

    *lv = b;
    //as->val = b;
    //as->initialized = true;
    sim_stack_push(b);
    
    return 0;
}