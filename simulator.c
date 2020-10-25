

#include <stdio.h>
#include <assert.h>

#include "simulator.h"

void ptree_evaluate_recursive(node *n);

static void generate_intermediate_code_recursive(node *n);
void resolve_jump_addresses(void);

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
int jumpz_op(void);
int addr_op(void);


typedef struct intermediate_spec_s
{
    ptree_tok_type type;
    int val;
} intermediate_spec;
intermediate_spec code[1000];
int ispec_index = 0;


int sim_stack[256];
int *sp = sim_stack;
int ip;

void generate_intermediate_code(node *n)
{
    ispec_index = 0;

    generate_intermediate_code_recursive(n);
    resolve_jump_addresses();
    //return code;
}

int jump_addr_table[1024];
static void generate_intermediate_code_recursive(node *n)
{
    for(int ci=0; n->children[ci]; ci++)
    {
        generate_intermediate_code_recursive(n->children[ci]);
    }

    if(n->type == SEMACT || n->type==NUMBER || n->type==VARIABLE || n->type==JMP_ADDR)
    {
        code[ispec_index].type = n->type;
        code[ispec_index].val = n->c;
        ispec_index++;
    }
    //else if(n->type == JUMP_ADDR)
    else if(n->type == JMP_LABEL)
    {
        jump_addr_table[n->c] = ispec_index;
    }
}

void resolve_jump_addresses(void)
{
    for(int i=0; i<ispec_index; i++)
    {
        /*intermediate_spec *instr = &code[ip];

        if(instr->type == JMP_ADDR)
        {
            //printf("changing ")
            instr->type = NUMBER;
            instr->val = jump_addr_table[instr->val];
        }*/

        if(code[i].type == JMP_ADDR)
        {
            //printf("found jmp addr placeholder, replacing w jump to addr %d\n", code[])
            code[i].type = NUMBER;
            code[i].val = jump_addr_table[code[i].val];
        }
    }
}

int run_intermediate_code(void)
{
    sp = sim_stack;
    ip = 0;

    bool jump_taken;

    //for(int i=0; i<ispec_index; i++)
    for(ip=0; ip<ispec_index; /*ip++*/)
    {
        printf("executing instruction %d of %d\n", ip, ispec_index);

        intermediate_spec *instr = &code[ip];
        jump_taken = false;

        if(instr->type == SEMACT)
        {
            //printf("semantic %c\n", n->c);
            switch(instr->val)
            {
                case ' ': case '(': case ')': break;
                case '+': add_op(); break;
                case '-': sub_op(); break;
                case '*': mult_op(); break;
                case '/': div_op(); break;
                case '%': mod_op(); break;
                case 'L': shl_op(); break;
                case 'R': shr_op(); break;
                case 'O': log_or_op(); break;
                case 'A': log_and_op(); break;
                case '&': bw_and_op(); break;
                case '|': bw_or_op(); break;
                case '^': bw_xor_op(); break;
                case 'E': eq_op(); break;
                case 'N': neq_op(); break;
                case 'g': geq_op(); break;
                case 'l': leq_op(); break;
                case ',': comma_op(); break;
                case '=': assign_op(); break;

                case 'j': if(jumpz_op()) jump_taken = true; break;
                //case 'a': addr_op(); break;
                //case '': _op(); break;

                default:  printf("unknown semantic action %c\n", instr->val); assert(0);
            }
        }
        else if(instr->type == NUMBER)
        {
            printf("pushing %d\n", instr->val);
            sim_stack_push(instr->val);
        }
        else if(instr->type == VARIABLE)    //only for rvalues. for lvalues, we want type NUMBER (push the address)
        {
            symbol *sym = (symbol *)instr->val;
            if(!sym->initialized) printf("--- warning: using uninitialized variable %s ---", sym->name);
            sim_stack_push(((symbol *)(instr->val))->val);
        }
        else assert(0);


        if(!jump_taken)
            ip++;
    }

    //pop the final result off the stack, return it
    int res = 0;
    if(sp != sim_stack)   //pure declarations (i.e. "int num;") don't create any value that goes on the stack
        res = sim_stack_pop();
    assert(sp >= sim_stack);
    return res;
}

void dump_intermediate(void)
{
    for(int i=0; i<ispec_index; i++)
    {
        printf("%0d\t", i);
        intermediate_spec *instr = &code[i];
        switch(instr->type)
        {
            case SEMACT:
                printf("SEMACT,\t  %c\n", instr->val);
                break;
            case NUMBER:
                printf("push %d\n", instr->val);
                break;
            case VARIABLE:
                printf("VARIABLE, %d\n", instr->val);
                break;
            default: printf("%s\n", instr->type==JMP_ADDR? "addr":"label"); assert(0);
        }
    }
}




int ptree_evaluate(node *n)
{
    sp = sim_stack;
    ptree_evaluate_recursive(n);

    int res = 0;
    if(sp != sim_stack)   //pure declarations (i.e. "int num;") don't create any value that goes on the stack
        res = sim_stack_pop();

    //printf("sp=%p, sim_stack=%p\n", sp, sim_stack);
    //assert(sp == sim_stack);
    assert(sp >= sim_stack);
    return res;
}

void ptree_evaluate_recursive(node *n)
{
    for(int ci=0; n->children[ci]; ci++)
    {
        ptree_evaluate_recursive(n->children[ci]);
    }

    /*if(n->type == SEMACT)
        printf("SEMACT,\t %c\n", n->c);
    else if(n->type == NUMBER)
        printf("NUMBER,\t %d\n", n->c);
    else if(n->type == VARIABLE)
        printf("VARIABLE,\t %d\n", ((symbol *)(n->c))->val);
    return;*/

    if(n->type == SEMACT)
    {
        //printf("semantic %c\n", n->c);
        switch(n->c)
        {
            case ' ': case '(': case ')': break;
            case '+': add_op(); break;
            case '-': sub_op(); break;
            case '*': mult_op(); break;
            case '/': div_op(); break;
            case '%': mod_op(); break;
            case 'L': shl_op(); break;
            case 'R': shr_op(); break;
            case 'O': log_or_op(); break;
            case 'A': log_and_op(); break;
            case '&': bw_and_op(); break;
            case '|': bw_or_op(); break;
            case '^': bw_xor_op(); break;
            case 'E': eq_op(); break;
            case 'N': neq_op(); break;
            case 'g': geq_op(); break;
            case 'l': leq_op(); break;
            case ',': comma_op(); break;
            case '=': assign_op(); break;
            //case '': _op(); break;

            default:  printf("unknown semantic action %c\n", n->c); assert(0);
        }
    }
    else if(n->type == NUMBER)
        sim_stack_push(n->c);
    else if(n->type == VARIABLE)    //only for rvalues. for lvalues, we want type NUMBER (push the address)
    {
        //printf("pushing variable val %d\n", ((symbol *)(n->c))->val);
        //if(!tp->sym->initialized) printf("--- warning: using uninitialized variable %s ---", tp->sym->name);

        symbol *sym = (symbol *)n->c;
        if(!sym->initialized) printf("--- warning: using uninitialized variable %s ---", sym->name);
        sim_stack_push(((symbol *)(n->c))->val);
    }
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
#define def_bin_op(name,op) \
  int name##_op(void)      \
  {                         \
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
    //int *p = (int*)stack_pop();
    //printf("assigning value %d to variable\n", b);

    symbol *as = (symbol *)sim_stack_pop();
    //int *val = as->val;

    //printf("simulator: var w symbol at addr %p, value is %d\n", as, as->val);

    //*val = b;
    as->val = b;
    as->initialized = true;
    sim_stack_push(b);

    return 0;
}

int jumpz_op(void)
{
    int addr = sim_stack_pop();
    int n = sim_stack_pop();
    if(n == 0)
    {
        printf("jumping to addr %d\n", addr);
        ip = addr;
        return 1;
    }
    else
        return 0;
}

int addr_op(void)
{
    assert(0);
    sim_stack_push(ip+3);
    printf("pushed addr %d\n", ip+3);

    return 0;
}
