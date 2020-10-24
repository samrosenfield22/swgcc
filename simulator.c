

#include <stdio.h>
#include <assert.h>

#include "simulator.h"

void ptree_evaluate_recursive(node *n);

void sim_stack_push(int n);
int sim_stack_pop(void);
void add_op(void);
void sub_op(void);
void mult_op(void);
void div_op(void);
void mod_op(void);
void shl_op(void);
void shr_op(void);
void leq_op(void);
void geq_op(void);
void eq_op(void);
void neq_op(void);
void bw_and_op(void);
void bw_or_op(void);
void bw_xor_op(void);
void log_and_op(void);
void log_or_op(void);
void comma_op(void);
void assign_op(void);

int sim_stack[256];
int *sp = sim_stack;

int ptree_evaluate(node *n)
{
    sp = sim_stack;
    ptree_evaluate_recursive(n);

    int res = 0;
    if(sp != sim_stack)   //pure declarations (i.e. "int num;") don't create any value that goes on the stack
        res = sim_stack_pop();
    assert(sp == sim_stack);
    return res;
}

void ptree_evaluate_recursive(node *n)
{
    for(int ci=0; n->children[ci]; ci++)
    {
        ptree_evaluate_recursive(n->children[ci]);
    }

    if(n->type == SEMACT)
    {
        printf("semantic %c\n", n->c);
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
  void name##_op(void)      \
  {                         \
      int b = sim_stack_pop();  \
      int a = sim_stack_pop();  \
      sim_stack_push(a op b);   \
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

void comma_op(void)
{
    int b = sim_stack_pop();
    sim_stack_pop();
    sim_stack_push(b);
}

void assign_op(void)
{
    int b = sim_stack_pop();
    //int *p = (int*)stack_pop();
    //printf("assigning value %d to variable\n", b);

    symbol *as = (symbol *)sim_stack_pop();
    //int *val = as->val;

    printf("simulator: var w symbol at addr %p, value is %d\n", as, as->val);

    //*val = b;
    as->val = b;
    as->initialized = true;
    sim_stack_push(b);
}
