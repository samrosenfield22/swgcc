

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "simulator.h"

static bool filter_semact(node *n);
static void generate_instruction(node *n, int depth);
static void dump_intermediate(void);
static void resolve_jump_addresses(void);

void sim_stack_push(int n);
int sim_stack_pop(void);

int add_op(void);
int sub_op(void);
int mult_op(void);
int div_op(void);
int mod_op(void);
int shl_op(void);
int shr_op(void);
int gt_op(void);
int lt_op(void);
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

int preinc_op(void);
int predec_op(void);
int addr_op(void);
int deref_op(void);
int log_not_op(void);
int bin_not_op(void);
int postinc_op(void);
int postdec_op(void);

int assign_eq_op(void);
int assign_add_op(void);
int assign_sub_op(void);
int assign_mult_op(void);
int assign_div_op(void);
int assign_mod_op(void);
int assign_shl_op(void);
int assign_shr_op(void);
int assign_and_op(void);
int assign_or_op(void);
int assign_xor_op(void);

int jmp_op(void);
int jz_op(void);
int jnz_op(void);


typedef struct intermediate_spec_s
{
    //ptree_tok_type type;
    char *op;
    int arg;
} intermediate_spec;
//intermediate_spec code[1000];
//int ispec_index = 0;
intermediate_spec *code = NULL;

unsigned char SIM_MEM[0x10000];
#define SIM_STACK_OFFSET    (0x0000)
#define SIM_VARS_OFFSET     (0x8000)    //

unsigned char *var_addr = SIM_MEM + SIM_VARS_OFFSET;

int sim_stack[256];
int *sp = sim_stack;
int ip;

void generate_intermediate_code(node *n)
{
    //ispec_index = 0;
    if(code) vector_destroy(code);
    code = vector(*code, 0);

    ptree_traverse_dfs(n, filter_semact, generate_instruction, true);

    printf("before resolving jumps:\n"); dump_intermediate();
    resolve_jump_addresses();
    printf("\n\nafter resolving jumps:\n"); dump_intermediate();
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
    vector_inc(&code);

    char *subs = strtok(n->str, " ");
    //code[ispec_index].op = strdup(subs);
    vector_last(code).op = strdup(subs);

    

    subs = strtok(NULL, " ");
    if(subs)
    {
        vector_last(code).arg = strtol(subs, NULL, 10);
        //code[ispec_index].arg = strtol(subs, NULL, 10);
        //printf(", arg=%d)\n", vector_last(code).arg);
    }
    //else
    //    printf(")\n");

    //printf("instruction %d\n\top: %s\n\targ: %d\n\n", ispec_index, code[ispec_index].op, code[ispec_index].arg);

	//ispec_index++;
}

static void dump_intermediate(void)
{
    vector_foreach(code,i)
    {
        printf("instruction %d (op: %s, arg %d)\n", i, code[i].op, code[i].arg);
    }
}

void resolve_jump_addresses(void)
{
    /*
    intermediate_spec *pushaddrs, *labels;
    pushaddrs = vector(*pushaddrs, 0);
    labels = vector(*labels, 0);
    int *laddrs = vector(*laddrs, 0);

    for(int i=0; i<vector_len(code); i++)
    {
        if(strcmp(code[i].op, "pushaddr")==0)
            vector_append(pushaddrs, code[i]);
        else if(strcmp(code[i].op, "jumplabel")==0)
        {
            vector_append(labels, code[i]);
            vector_append(laddrs, i);
        }
    }

    for(int i=0; i<vector_len(pushaddrs))
    {
        for(int j=0; j<vector_len(labels); j++)
        {
            if(pushaddrs[i].arg == labels[j].arg)
            {
                //update stuff


                //delete
                vector_delete(pushaddrs, i);
                vector_delete(labels, j);
                i--; j--;
            }
        }
    }

    vector_destroy(pushaddrs);
    vector_destroy(labels);
    */

    printf("------------------------\nresolving jump addresses (%d total instrs)\n", vector_len(code));
    for(int i=0; i<vector_len(code); i++)
    {
        //printf("\t%d %s\n", i, code[i].op);
        if(strcmp(code[i].op, "pushaddr")==0)
        {
            //find the matching jump label
            int skipped_labels = 0;
            for(int j=0; j<vector_len(code); j++)
            {
                
                if(strcmp(code[j].op, "jumplabel")==0)
                {
                    if(code[i].arg==code[j].arg)
                    {
                        //printf("pushaddr at %d, jumplabel at %d\n", i, j);
                        //exit(0);
                        free(code[i].op);
                        //char buf[41];
                        //snprintf(buf, 40, "push %d", j);
                        //code[i].op = strdup(buf);
                        code[i].op = strdup("push");
                        code[i].arg = j - skipped_labels;

                        vector_delete(&code, j);
                    }
                    else
                        skipped_labels++;
                }
            }
        }
    }
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
    {"<=",  leq_op},
    {">",   gt_op},
    {"<",   lt_op},
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
    {"*deref",  deref_op},
    {"!",    log_not_op},
    {"~",    bin_not_op},

    {"++post",    postinc_op},
    {"--post",    postdec_op},

    {"=",   assign_eq_op},
    {"+=",    assign_add_op},
    {"-=",    assign_sub_op},
    {"*=",    assign_mult_op},
    {"/=",    assign_div_op},
    {"%=",    assign_mod_op},
    {"<<=",    assign_shl_op},
    {">>=",    assign_shr_op},
    {"&=",    assign_and_op},
    {"|=",    assign_or_op},
    {"^=",    assign_xor_op},

    {"jmp",     jmp_op},
    {"jz",      jz_op},
    {"jnz",      jnz_op}
    
    //{"",    _op},
    
};

//if (expr) {push jumpaddr} {jumpz semact} stmtlist {jmp label}
//if (expr) {jz semact} stmtlist {jumplabel}

int run_intermediate_code(void)
{
    sp = sim_stack;
    ip = 0;

    bool jump_taken;

    printf("\n-------------------\nexecution dump:\n");

    printf("before:\t");
    dump_symbol_table_oneline();

    //for(ip=0; ip<ispec_index; ip++)
    for(ip=0; ip<vector_len(code); /*ip++*/)
    {
        //intermediate_spec *instr = &code[ip];
        intermediate_spec *instr = &code[ip];
        jump_taken = false;

        printf("%03d %s\t", ip, instr->op);

        if(strcmp(instr->op, "push")==0)
            sim_stack_push(instr->arg);
        else if(strcmp(instr->op, "pushv")==0)
            sim_stack_push(*(int*)instr->arg);
        else if(strcmp(instr->op, "pop")==0)
            sim_stack_pop();    //what do with result?
        else
        {
            for(int i=0; i<sizeof(op_table)/sizeof(op_table[0]); i++)
            {
                if(strcmp(instr->op, op_table[i].op)==0)
                {
                    int res = op_table[i].func();

                    //if(op is a jump op)
                    if(res == 1)    //right now, only the jmp ops return anything (other than 0)
                                    //1 means a jump was taken -- so we don't increment ip
                        jump_taken = true;

                    break;
                }
            }
        }

        dump_symbol_table_oneline();
        printf("\n");

        if(!jump_taken)
            ip++;
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

#define def_assign_op(name,op)              \
    int assign_##name##_op(void)            \
    {                                       \
        int b = sim_stack_pop();            \
        int *lv = (int*)sim_stack_pop();    \
        *lv op b;                           \
        sim_stack_push(*lv);                  \
        return 0;                           \
    }

#define BY_VALUE true
#define BY_REFERENCE false

#define def_jump_op(name, cond)             \
    int name##_op(void)                     \
    {                                       \
        int jaddr = sim_stack_pop();        \
        int arg = sim_stack_pop();          \
                                            \
        if(arg cond)                        \
        {                                   \
            ip = jaddr;                     \
            return 1;                       \
        }                                   \
        else                                \
            return 0;                       \
    }

def_binary_op(add, +)
def_binary_op(sub, -)
def_binary_op(mult, *)
def_binary_op(div, /)
def_binary_op(mod, %)
def_binary_op(shl, <<)
def_binary_op(shr, >>)
def_binary_op(lt, <)
def_binary_op(gt, >)
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
def_unary_prefix_op(deref, , BY_VALUE)
def_unary_prefix_op(log_not, !, BY_VALUE)
def_unary_prefix_op(bin_not, ~, BY_VALUE)

def_unary_postfix_op(postinc, ++)
def_unary_postfix_op(postdec, --)

def_assign_op(eq, =)
def_assign_op(add, +=)
def_assign_op(sub, -=)
def_assign_op(mult, *=)
def_assign_op(div, /=)
def_assign_op(mod, %=)
def_assign_op(shl, <<=)
def_assign_op(shr, >>=)
def_assign_op(and, &=)
def_assign_op(or, |=)
def_assign_op(xor, ^=)

def_jump_op(jz, == 0)
def_jump_op(jnz, != 0)



int comma_op(void)
{
    int b = sim_stack_pop();
    sim_stack_pop();    //throw away value
    sim_stack_push(b);
    return 0;
}

/*int assign_op(void)
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
}*/

int jmp_op(void)
{
    ip = sim_stack_pop();
    return 1;
}