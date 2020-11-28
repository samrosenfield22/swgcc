

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "simulator.h"


int sim_stack_push(int n);
int sim_stack_pushv(int n);
int sim_stack_pushl(int n);
int sim_stack_pushlv(int n);
int sim_stack_pop(int);

int incsp(int bytes);
int decsp(int bytes);

int add_op(int);
int sub_op(int);
int mult_op(int);
int div_op(int);
int mod_op(int);
int shl_op(int);
int shr_op(int);
int gt_op(int);
int lt_op(int);
int leq_op(int);
int geq_op(int);
int eq_op(int);
int neq_op(int);
int bw_and_op(int);
int bw_or_op(int);
int bw_xor_op(int);
int log_and_op(int);
int log_or_op(int);
int comma_op(int);

int preinc_op(int);
int predec_op(int);
int addr_op(int);
int deref_op(int);
int log_not_op(int);
int bin_not_op(int);
int postinc_op(int);
int postdec_op(int);

int assign_eq_op(int);
int assign_add_op(int);
int assign_sub_op(int);
int assign_mult_op(int);
int assign_div_op(int);
int assign_mod_op(int);
int assign_shl_op(int);
int assign_shr_op(int);
int assign_and_op(int);
int assign_or_op(int);
int assign_xor_op(int);

int jmp_op(int);
int jz_op(int);
int jnz_op(int);



char sim_stack[1024];

bool jump_taken;//, declaration_only;

//external variables (declared in simulator.h)
char SIM_MEM[0x10000];
char *sp = sim_stack, *bp = sim_stack;
int eax;
intermediate_spec *ip, *ip_start=(intermediate_spec *)(SIM_MEM + SIM_CODE_OFFSET), *ip_end;



/*char *get_var_addr(symbol *variable)
{
    assert(0);
    if(variable->lifetime == STATIC)
        return variable->var;
    else
        return (int)bp + variable->var;
}*/

void *get_code_addr(void)
{
    return ip_start;
}


struct op_entry
{
    char *op;
    int (*func)(int);
} op_table[] =
{
    {"push",    sim_stack_push},
    {"pushv",    sim_stack_pushv},
    {"pushl",   sim_stack_pushl},
    {"pushlv",   sim_stack_pushlv},
    {"pop",    sim_stack_pop},

    {"incsp",   incsp},
    {"decsp",   decsp},

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


int run_intermediate_code(bool verbose)
{
    //verbose = true;
    jump_taken = false;
    int res = 0;
    const int dump_spaces = 24;

    int cursor;
    if(verbose)
    {
        printf("\n-------------------\nexecution dump:\n");
        cursor = printf("before: ");
        while(cursor++ < dump_spaces) putchar(' ');
        dump_symbol_table_oneline();
        printf("\n");
    }

    for(ip=ip_start; ip<ip_end; )
    {
        jump_taken = false;
        cursor = 0;

        if(verbose)     //(main+8)push a
        {
            cursor += print_instr(ip);
            while(cursor++ < dump_spaces) putchar(' ');
        }

        //execute the instruction
        bool valid_instr = false;
        for(int i=0; i<sizeof(op_table)/sizeof(op_table[0]); i++)
        {
            if(strcmp(ip->op, op_table[i].op)==0)
            {
                res = op_table[i].func(ip->arg);     //the arg is a dummy value for most instructions
                valid_instr = true;
                break;
            }
        }
        if(!valid_instr)
        {
            printfcol(RED_FONT, "unrecognized instruction: %s\n", ip->op);
            //exit(-1);
        }

        if(verbose)     //bp=0 sp=4     (num main+56 0)
            dump_stack();

        if(!jump_taken)
            ip++;
        jump_taken = false;

        //make sure we're not out of bounds
        if(ip > ip_end)
        {
            printfcol(RED_FONT, "error: ip past end of code\n");
            printf("ip:\t\t%d\t", (int)ip);             print_reg_or_val((int)ip);
            printf("\nip_start:\t%d\t", (int)ip_start);   print_reg_or_val((int)ip_start);
            printf("\nip_end:\t\t%d\t", (int)ip_end);     print_reg_or_val((int)ip_end);
            printf("\n");
            print_reg_or_val((int)((char*)ip-(char*)ip_start));
            printf("\n");

            //dump_intermediate();
            assert(0);
        }
    }

    //execution_done:
    ip_start = ip;
    if(sp != sim_stack)
    {
        printf("--- sp not at stack head, it's at %d ---\n", sp-sim_stack);
        assert(0);
    }
    //return sim_stack_pop(0);
    return res;
}

//if we're only doing a decl (i.e. if the code is a function declaration), we generate it but don't execute it --
//we call this instead of run_intermediate_code()
void skip_code(void)
{
    ip = ip_start = ip_end;
}

int print_instr(intermediate_spec *instr)
{
    //
    printf("(");
    int cursor = print_reg_or_val((int)instr) + 2;
    printf(")");

    //
    cursor += printf("%s ", instr->op);
    if(!(strcmp(instr->op, "push")==0 || strcmp(instr->op, "pushv")==0 || 
        strcmp(instr->op, "pushl")==0 || strcmp(instr->op, "pushlv")==0 || 
        strcmp(instr->op, "pop")==0 || strcmp(instr->op, "incsp")==0 || strcmp(instr->op, "decsp")==0 || 
        strcmp(instr->op, "pushaddr")==0 || strcmp(instr->op, "jumplabel")==0))  //in case we dump before resolving 
        return cursor;

    return cursor + print_reg_or_val(instr->arg);
}

int print_reg_or_val(int arg)
{
    char *carg = (char*)arg;

    symbol *sym = symbol_search_by_addr(carg);
    symbol *func = symbol_search_function_addr(carg);
    
    //else if(ip->arg == (int)&ip) return printf("ip");
    if(arg == (int)&bp)                     return printf("bp");
    else if(arg == (int)&sp)                return printf("sp");
    else if(arg == (int)&eax)               return printf("eax");
    else if(sym)                            return printfcol(YELLOW_FONT, "%s", sym->name);
    else if(carg >= (char*)ip_start)  return printfcol(YELLOW_FONT, "main+%03d", (int)(carg - (char*)ip_start));
    else if(func)     return printfcol(YELLOW_FONT, "%s+%d", func->name, carg - (char*)(func->var));
    else if(sim_stack<=carg && carg<=sp) return printf("stack+%d", carg-sim_stack);
    else return printf("%d", arg);
}

void dump_stack(void)
{
    printf("\tbp=%d sp=%d\t(", bp-sim_stack, sp-sim_stack);
    for(char *p=sim_stack; p<sp; p+=4)
    {
        print_reg_or_val(*(int*)p);
        printf(" ");
    }
    printf(")\n");
}


///////////////////////

int sim_stack_push(int n)
{
    memcpy(sp, &n, 4);
    sp += 4;
    return 0;
}

int sim_stack_pushv(int n)
{
    //*sp++ = *(int*)n;
    int pushv = *(int*)n;
    //*sp = pushv;
    memcpy(sp, &pushv, 4);
    sp += 4;
    return 0;
}

int sim_stack_pushl(int n)
{
    int local = (int)(bp + n);
    //*sp = (int)local;
    memcpy(sp, &local, 4);
    sp += 4;
    return 0;
}

int sim_stack_pushlv(int n)
{
    int local = (int)(bp + n);
    int pushv = *(int*)local;
    //*sp = pushv;
    memcpy(sp, &pushv, 4);
    sp += 4;
    return 0;
}

int sim_stack_pop(int d)
{
    assert(sp > sim_stack);    //underflow
    sp -= 4;
    //int popval = *(int*)sp;
    int popval;
    memcpy(&popval, sp, 4);
    if(d)
        *(int*)d = popval;
    return popval;
}

int incsp(int bytes)
{
    assert(bytes > 0);
    sp += bytes;
    return 0;
}

int decsp(int bytes)
{
    assert(bytes > 0);
    sp -= bytes;
    return 0;
}

//all binary operators follow the same semantic action format
#define def_binary_op(name,op)     \
    int name##_op(int d)      \
    {                             \
        int b = sim_stack_pop(0);  \
        int a = sim_stack_pop(0);  \
        sim_stack_push(a op b);   \
        return 0;                 \
    }

#define def_unary_prefix_op(name,op,by_val)     \
    int name##_op(int d)      \
    {                             \
        int a = sim_stack_pop(0);  \
        if(by_val)                      \
            sim_stack_push((int)op(*(int*)a));     \
        else                                \
            sim_stack_push((int)op(a));     \
        return 0;                 \
    }

#define def_unary_postfix_op(name,op)     \
    int name##_op(int d)      \
    {                             \
        int *a = (int*)sim_stack_pop(0);  \
        sim_stack_push((*a)op);     \
        return 0;                 \
    }

#define def_assign_op(name,op)              \
    int assign_##name##_op(int d)            \
    {                                       \
        int b = sim_stack_pop(0);            \
        int *lv = (int*)sim_stack_pop(0);    \
        *lv op b;                           \
        sim_stack_push(*lv);                \
        return 0;                           \
    }

#define BY_VALUE true
#define BY_REFERENCE false

#define def_jump_op(name, cond)             \
    int name##_op(int d)                     \
    {                                       \
        int jaddr = sim_stack_pop(0);        \
        int arg = sim_stack_pop(0);          \
                                            \
        if(arg cond)                        \
        {                                   \
            ip = (intermediate_spec *)jaddr;                     \
            jump_taken = true;                       \
        }                                   \
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



int comma_op(int d)
{
    int b = sim_stack_pop(0);
    sim_stack_pop(0);    //throw away value
    sim_stack_push(b);
    return 0;
}

int jmp_op(int d)
{
    ip = (intermediate_spec *)sim_stack_pop(0);
    jump_taken = true;
    return 0;
}