

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "simulator.h"

long nop(long dummy);

long sim_stack_push(long n);
long sim_stack_pushv(long n);
long sim_stack_pushl(long n);
long sim_stack_pushlv(long n);
long sim_stack_pop(long);

long incsp(long bytes);
long decsp(long bytes);

long add_op(long);
long sub_op(long);
long mult_op(long);
long div_op(long);
long mod_op(long);
long shl_op(long);
long shr_op(long);
long gt_op(long);
long lt_op(long);
long leq_op(long);
long geq_op(long);
long eq_op(long);
long neq_op(long);
long bw_and_op(long);
long bw_or_op(long);
long bw_xor_op(long);
long log_and_op(long);
long log_or_op(long);
long comma_op(long);

long preinc_op(long);
long predec_op(long);
long addr_op(long);
long deref_op(long);
long log_not_op(long);
long bin_not_op(long);
long postinc_op(long);
long postdec_op(long);

long assign_eq_op(long);
long assign_add_op(long);
long assign_sub_op(long);
long assign_mult_op(long);
long assign_div_op(long);
long assign_mod_op(long);
long assign_shl_op(long);
long assign_shr_op(long);
long assign_and_op(long);
long assign_or_op(long);
long assign_xor_op(long);

long jmp_op(long);
long jz_op(long);
long jnz_op(long);



char sim_stack[1024];

bool jump_taken;//, declaration_only;

//external variables (declared in simulator.h)
char SIM_MEM[0x10000];
char *sp = sim_stack, *bp = sim_stack;
long eax;
intermediate_spec *ip, *ip_start=(intermediate_spec *)(SIM_MEM + SIM_CODE_OFFSET), *ip_end;



/*char *get_var_addr(symbol *variable)
{
    assert(0);
    if(variable->lifetime == STATIC)
        return variable->var;
    else
        return (long)bp + variable->var;
}*/

void *get_code_addr(void)
{
    return ip_start;
}


struct op_entry
{
    char *op;
    long (*func)(long);
} op_table[] =
{
    {"nop",     nop},
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


long run_intermediate_code(bool verbose)
{
    //verbose = true;
    jump_taken = false;
    long res = 0;
    const long dump_spaces = 24;

    long cursor;
    if(verbose)
    {
        printf("\n-------------------\nexecution dump:\n");
        cursor = printf("before: ");
        while(cursor++ < dump_spaces) putchar(' ');
        dump_symbol_table_oneline();
        printf("\n");
    }

    if(verbose)
    {
        printf("instruction");
        for(long i=strlen("instruction"); i<dump_spaces; i++) putchar(' ');
        printf("regs\t\tstack\n");
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
        for(long i=0; i<sizeof(op_table)/sizeof(op_table[0]); i++)
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
            printf("ip:\t\t%ld\t", (long)ip);             print_reg_or_val((long)ip);
            printf("\nip_start:\t%ld\t", (long)ip_start);   print_reg_or_val((long)ip_start);
            printf("\nip_end:\t\t%ld\t", (long)ip_end);     print_reg_or_val((long)ip_end);
            printf("\n");
            print_reg_or_val((long)((char*)ip-(char*)ip_start));
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

long print_instr(intermediate_spec *instr)
{
    //
    printf("(");
    long cursor = print_reg_or_val((long)instr) + 2;
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

long print_reg_or_val(long arg)
{
    char *carg = (char*)arg;

    symbol *sym = symbol_search_by_addr(carg);
    symbol *func = symbol_search_function_addr(carg);
    
    //else if(ip->arg == (long)&ip) return printf("ip");
    if(arg == (long)&bp)                     return printf("bp");
    else if(arg == (long)&sp)                return printf("sp");
    else if(arg == (long)&eax)               return printf("eax");
    else if(sym)                            return printfcol(YELLOW_FONT, "%s", sym->name);
    else if(sim_stack<=carg && carg<=sp)    return printf("bp+%d", carg-bp);
    else if(carg >= (char*)ip_start)  return printfcol(YELLOW_FONT, "main+%03ld", (long)(carg - (char*)ip_start));
    else if(func)     return printfcol(YELLOW_FONT, "%s+%d", func->name, carg - (char*)(func->var));
    //else if(sim_stack<=carg && carg<=sp) return printf("stack+%ld", carg-sim_stack);
    else return printf("%ld", arg);
}

void dump_stack(void)
{
    printf("bp=%d sp=%d\t(", bp-sim_stack, sp-sim_stack);
    for(char *p=sim_stack; p<sp; p+=4)
    {
        print_reg_or_val(*(long*)p);
        printf(" ");
    }
    printf(")\n");
}


///////////////////////

long nop(long dummy)
{
    return 0;
}

long sim_stack_push(long n)
{
    memcpy(sp, &n, 4);
    sp += 4;
    return 0;
}

long sim_stack_pushv(long n)
{
    //*sp++ = *(long*)n;
    long pushv = *(long*)n;
    //*sp = pushv;
    memcpy(sp, &pushv, 4);
    sp += 4;
    return 0;
}

long sim_stack_pushl(long n)
{
    long *local = (long*)(bp + n);
    //*sp = (long)local;
    memcpy(sp, &local, 4);
    sp += 4;
    return 0;
}

long sim_stack_pushlv(long n)
{
    long *local = (long*)(bp + n);
    long pushv = *local;
    //*sp = pushv;
    memcpy(sp, &pushv, 4);
    sp += 4;
    return 0;
}

long sim_stack_pop(long d)
{
    assert(sp > sim_stack);    //underflow
    sp -= 4;
    //long popval = *(long*)sp;
    long popval;
    memcpy(&popval, sp, 4);
    if(d)
        *(long*)d = popval;
    return popval;
}

long incsp(long bytes)
{
    assert(bytes > 0);
    sp += bytes;
    return 0;
}

long decsp(long bytes)
{
    assert(bytes > 0);
    sp -= bytes;
    return 0;
}

//all binary operators follow the same semantic action format
#define def_binary_op(name,op)     \
    long name##_op(long d)      \
    {                             \
        long b = sim_stack_pop(0);  \
        long a = sim_stack_pop(0);  \
        sim_stack_push(a op b);   \
        return 0;                 \
    }

#define def_unary_prefix_op(name,op,by_val)     \
    long name##_op(long d)      \
    {                             \
        long a = sim_stack_pop(0);  \
        if(by_val)                      \
            sim_stack_push((long)op(*(long*)a));     \
        else                                \
            sim_stack_push((long)op(a));     \
        return 0;                 \
    }

#define def_unary_postfix_op(name,op)     \
    long name##_op(long d)      \
    {                             \
        long *a = (long*)sim_stack_pop(0);  \
        sim_stack_push((*a)op);     \
        return 0;                 \
    }

#define def_assign_op(name,op)              \
    long assign_##name##_op(long d)            \
    {                                       \
        long b = sim_stack_pop(0);            \
        long *lv = (long*)sim_stack_pop(0);    \
        *lv op b;                           \
        sim_stack_push(*lv);                \
        return 0;                           \
    }

#define BY_VALUE true
#define BY_REFERENCE false

#define def_jump_op(name, cond)             \
    long name##_op(long d)                     \
    {                                       \
        long jaddr = sim_stack_pop(0);        \
        long arg = sim_stack_pop(0);          \
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



long comma_op(long d)
{
    long b = sim_stack_pop(0);
    sim_stack_pop(0);    //throw away value
    sim_stack_push(b);
    return 0;
}

long jmp_op(long d)
{
    ip = (intermediate_spec *)sim_stack_pop(0);
    jump_taken = true;
    return 0;
}