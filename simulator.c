

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

int preinc_op(void);
int predec_op(void);
int addr_op(void);
int log_not_op(void);
int bin_not_op(void);
int postinc_op(void);
int postdec_op(void);


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
    {"+",   add_op},
    {"-",   sub_op},
    {"*",   mult_op},
    {"/",   div_op},
    {"%",   mod_op},
    {"<<",  shl_op},
    {">>",  shr_op},
    {"=",   assign_op},
    {"<=",  leq_op},
    {">=",    geq_op},
    {"==",    eq_op},
    {"!=",    neq_op},
    {"|",    bw_or_op},
    {"&",    bw_and_op},
    {"^",    bw_xor_op},
    {"||",    log_or_op},
    {"&&",    log_and_op},

    {"++pre",    preinc_op},
    {"--pre",    predec_op},
    {"&addr",    addr_op},
    {"!",    log_not_op},
    {"~",    bin_not_op},

    {"++post",    postinc_op},
    {"--post",    postdec_op}
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
        else if(strcmp(instr->op, "pushp")==0)
            sim_stack_push(*(int*)instr->arg);
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
#define def_binary_op(name,op)     \
    int name##_op(void)      \
    {                             \
        int b = sim_stack_pop();  \
        int a = sim_stack_pop();  \
        sim_stack_push(a op b);   \
        return 0;                 \
    }

#define def_unary_prefix_op(name,op,by_val)     \
    int name##_op(void)      \
    {                             \
        int a = sim_stack_pop();  \
        if(by_val)                      \
            sim_stack_push((int)op(*(int*)a));     \
        else                                \
            sim_stack_push((int)op(a));     \
        return 0;                 \
    }

#define def_unary_postfix_op(name,op)     \
    int name##_op(void)      \
    {                             \
        int *a = (int*)sim_stack_pop();  \
        sim_stack_push((*a)op);     \
        return 0;                 \
    }

#define BY_VALUE true
#define BY_REFERENCE false

def_binary_op(add, +)
def_binary_op(sub, -)
def_binary_op(mult, *)
def_binary_op(div, /)
def_binary_op(mod, %)
def_binary_op(shl, <<)
def_binary_op(shr, >>)
def_binary_op(leq, <=)
def_binary_op(geq, >=)
def_binary_op(eq, ==)
def_binary_op(neq, !=)
def_binary_op(bw_or, |)
def_binary_op(bw_and, &)
def_binary_op(bw_xor, ^)
def_binary_op(log_or, ||)
def_binary_op(log_and, &&)
//def_binary_op(comma, ,)
//def_binary_op(,)

def_unary_prefix_op(preinc, ++, BY_VALUE)
def_unary_prefix_op(predec, --, BY_VALUE)
def_unary_prefix_op(addr, , BY_REFERENCE)  //uhhh
def_unary_prefix_op(log_not, !, BY_VALUE)
def_unary_prefix_op(bin_not, ~, BY_VALUE)

def_unary_postfix_op(postinc, ++)
def_unary_postfix_op(postdec, --)

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