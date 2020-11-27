

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "simulator.h"

typedef struct intermediate_spec_s
{
    //ptree_tok_type type;
    char *op;
    int arg;
} intermediate_spec;

static bool filter_semact(node *n);
static void generate_instruction(node *n, int depth);
static void generate_instruction_from_str(char *str);
static void resolve_jump_addresses(void);

static int print_instr(intermediate_spec *instr);
static int print_reg_or_val(int arg);

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



//intermediate_spec code[1000];
//int ispec_index = 0;
intermediate_spec *code = NULL;

char SIM_MEM[0x10000];
#define SIM_STACK_OFFSET    (0x0000)
#define SIM_CODE_OFFSET     (0x1000)
#define SIM_VARS_OFFSET     (0x8000)    //

char *var_addr = SIM_MEM + SIM_VARS_OFFSET;
int local_var_addr = 0;     //bp offsets

char sim_stack[1024];
char *sp = sim_stack, *bp = sim_stack;
int eax;
//int ip;

intermediate_spec *ip, *ip_start=(intermediate_spec *)(SIM_MEM + SIM_CODE_OFFSET), *ip_end;
bool jump_taken;//, declaration_only;

typedef struct meta_op_s
{
    char *mnemonic;
    char **ops;
} meta_op;

meta_op META_OPS[] =
{
    {"enter",   (char*[]){"pushv bp", "pushv sp", "pop bp", NULL}},   //push bp; bp = sp
    {"leave",   (char*[]){"pushv bp", "pop sp", "pop bp", NULL}},      //sp = bp; pop bp
    //{"enter", (char*[]){NULL}},
    //{"leave", (char*[]){NULL}},
    {"ret",     (char*[]){"jmp", NULL}}    //or "pop ip"
    //{"call",    (char*[]){"pushaddr 3", "jmp", NULL}}    //the pushv ret gets swapped w the previous instruction
                                                        //(the base_id push)
};

static meta_op *meta_op_lookup(const char *mop)
{
    for(int i=0; i<sizeof(META_OPS)/sizeof(META_OPS[0]); i++)
    {
        if(strcmp(mop, META_OPS[i].mnemonic)==0)
            return &META_OPS[i];
    }
    return NULL;
}

void generate_intermediate_code(node *n)
{
    if(code) vector_destroy(code);
    code = vector(*code, 0);

    //declaration_only = false;

    local_var_addr = 0;

    ptree_traverse_dfs(n, filter_semact, generate_instruction, -1, true);

    //printf("~~~ instructions before resolving jumps ~~~\n");
    //dump_intermediate();

    resolve_jump_addresses();

    intermediate_spec *cp = ip_start;
    vector_foreach(code, i)
    {
        cp->op = strdup(code[i].op);
        cp->arg = code[i].arg;
        cp++;
    }
    ip_end = cp;
}

int define_var(symbol *sym)
{
    //get number of bytes
    symbol *type = sym->type;
    size_t bytes = type->tspec->bytes;

    //allocate the var
    if(sym->lifetime == STATIC)
    {
        sym->var = (int*)var_addr;
        var_addr += bytes;
        return 0;
    }
    else
    {
        sym->var = (int*)local_var_addr;
        local_var_addr += bytes;
        return bytes;
    }
}

int *get_var_addr(symbol *variable)
{
    if(variable->lifetime == STATIC)
        return variable->var;
    else
        return (int)bp + (char*)variable->var;
}

void *get_code_addr(void)
{
    return ip_start;
}

static bool filter_semact(node *n)
{
	return (!(n->is_nonterminal) && n->ntype == SEMACT);
}

static void generate_instruction(node *n, int depth)
{
    //when we define a function, the function name is a base_id which gets pushed onto the stack.
    //we don't want this
    if(strcmp(n->str, "enter")==0)
    {
        vector_delete(&code, vector_len(code)-1);
    }
    
    meta_op *meta = meta_op_lookup(n->str);
    if(meta)
    {
        //declaration_only = true;
        for(char **p=meta->ops; *p; p++)
            generate_instruction_from_str(*p);
    }
    else
        generate_instruction_from_str(n->str);
}

static void generate_instruction_from_str(char *str)
{
    vector_inc(&code);
    char *scpy = strdup(str);

    char *subs = strtok(scpy, " ");
    vector_last(code).op = strdup(subs);    

    subs = strtok(NULL, " ");
    if(subs)
    {
        if(strcmp(subs, "bp")==0)       vector_last(code).arg = (int)&bp;
        else if(strcmp(subs, "sp")==0)  vector_last(code).arg = (int)&sp;
        else if(strcmp(subs, "ip")==0)  vector_last(code).arg = (int)&ip;
        //else if(strcmp(subs, "ret")==0)  vector_last(code).arg = (int)(ip_start + vector_len(code) + 3-2);
            //a func call takes 3 instrs. but by the time we're generating this instruction, we've already
            //made 2 extra instructions
        else if(strcmp(subs, "eax")==0)  vector_last(code).arg = (int)&eax;
        //else if(strcmp(subs, "ret")==0)  vector_last(code).arg = 123456;
        else vector_last(code).arg = strtol(subs, NULL, 10);
    }
    else
        vector_last(code).arg = 0;

    free(scpy);
}

void dump_intermediate(void)
{
    //int code_region_offset = (int)(ip_start - (intermediate_spec *)(SIM_MEM+SIM_CODE_OFFSET));
    //printf("bp:\taddr %d\tval %d\n", (int)&bp, (int)bp);
    //printf("sp:\taddr %d\tval %d\n", (int)&sp, (int)sp);
    //printf("ip:\taddr %d\tval %d\n", (int)&ip, (int)ip);
    
    vector_foreach(code,i)
    {
        printf("instruction %02d (", i);        //instruction index
        print_reg_or_val((int)(&ip_start[i]));  //instruction address
        printf(")(");
        print_instr(&code[i]);                  //instruction (op, and arg if there is one)
        printf(")\n");
    }
}

void resolve_jump_addresses(void)
{
    int *labels = vector(*labels, 0);
    
    //get all label addresses
    vector_foreach(code, i)
    {
        if(strcmp(code[i].op, "jumplabel")==0)
        {
            while(vector_len(labels) <= code[i].arg)
                vector_append(labels, 0);
            assert(labels[code[i].arg] == 0);       //there must not be multiple labels with the same val
            labels[code[i].arg] = (int)&(ip_start[i]);
            vector_delete(&code, i);
        }
    }

    //update all jmps with the correct address
    vector_foreach(code, i)
    {
        if(strcmp(code[i].op, "pushaddr")==0)
        {
            free(code[i].op);
            code[i].op = strdup("push");
            if(labels[code[i].arg] == 0)
            {
                printf("trying to push jump addr %d\n", i);
            }
            assert(labels[code[i].arg] != 0);       //the label with that id must exist
            code[i].arg = labels[code[i].arg];
        }
    }

    vector_destroy(labels);
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

    /*if(declaration_only)
    {
        printf("declaration only, not executing code\n");
        res = (int)ip_start;
        ip = ip_end;
        goto execution_done;
    }*/

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
        //intermediate_spec *instr = &code[ip];
        jump_taken = false;

        if(verbose)
        {
            //cursor = printf("(%d) ", (int)ip);
            printf("(");
            cursor = print_reg_or_val((int)ip) + 2;
            printf(")");
            cursor += print_instr(ip);
            while(cursor++ < dump_spaces) putchar(' ');
        }

        //execute the instruction
        /*if(strcmp(ip->op, "pushl")==0 || strcmp(ip->op, "pushlv")==0)
                {
                    printf("aaaaaaaaa\n");
                    //dump_intermediate();
                    //dump_symbol_table();
                    getchar();
                }*/
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

        if(verbose)
        {
            //dump_symbol_table_oneline();
            printf("\tbp=%d sp=%d\t(", bp-sim_stack, sp-sim_stack);
            for(char *p=sim_stack; p<sp; p+=4)
            {
                //printf("%d ", *(int*)p);
                print_reg_or_val(*(int*)p);
                printf(" ");
            }
            printf(")\n");
        }

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

            dump_intermediate();
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

static int print_instr(intermediate_spec *instr)
{
    int cursor = printf("%s ", instr->op);
    if(!(strcmp(instr->op, "push")==0 || strcmp(instr->op, "pushv")==0 || 
        strcmp(instr->op, "pushl")==0 || strcmp(instr->op, "pushlv")==0 || 
        strcmp(instr->op, "pop")==0 || strcmp(instr->op, "incsp")==0 || strcmp(instr->op, "decsp")==0 || 
        strcmp(instr->op, "pushaddr")==0 || strcmp(instr->op, "jumplabel")==0))  //in case we dump before resolving 
        return cursor;

    return cursor + print_reg_or_val(instr->arg);
}

static int print_reg_or_val(int arg)
{
    symbol *sym = symbol_search_by_addr((int*)arg);
    symbol *func = symbol_search_function_addr((int*)arg);
    
    //else if(ip->arg == (int)&ip) return printf("ip");
    if(arg == (int)&bp)                     return printf("bp");
    else if(arg == (int)&sp)                return printf("sp");
    else if(arg == (int)&eax)               return printf("eax");
    else if(sym)                            return printfcol(YELLOW_FONT, "%s", sym->name);
    else if((char*)arg >= (char*)ip_start)  return printfcol(YELLOW_FONT, "main+%d", (int)((char*)arg - (char*)ip_start));
    else if(func)     return printfcol(YELLOW_FONT, "%s+%d", func->name, (char*)arg - (char*)(func->var));
    else if(sim_stack<=(char*)arg && (char*)arg<=sp) return printf("stack+%d", (char*)(arg)-sim_stack);
    else return printf("%d", arg);
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