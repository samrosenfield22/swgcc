

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "intermediate.h"

typedef struct meta_op_s
{
    char *mnemonic;
    char **ops;
} meta_op;

//
static void generate_instruction(void *n);
static void generate_instruction_from_str(char *str);
static void resolve_jump_addresses(void);
static meta_op *meta_op_lookup(const char *mop);
static bool filter_semact(void *n);

//
intermediate_spec *code = NULL;

int local_var_addr = 0, local_arg_addr = -12;     //bp offsets




void generate_intermediate_code(pnode *n)
{
    if(code) vector_destroy(code);
    code = vector(*code, 0);

    //declaration_only = false;

    local_var_addr = 0;
    local_arg_addr = -12;

    //make instructions for all semantics
    tree_traverse(n, if(filter_semact(n)) generate_instruction(n), true);

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

static void generate_instruction(void *n)
{
    pnode *nn = (pnode *)n;

    //when we define a function, the function name is a base_id which gets pushed onto the stack.
    //we don't want this
    if(strcmp(nn->str, "enter")==0)
    {
        vector_delete(&code, vector_len(code)-1);
    }
    
    meta_op *meta = meta_op_lookup(nn->str);
    if(meta)
    {
        //declaration_only = true;
        for(char **p=meta->ops; *p; p++)
            generate_instruction_from_str(*p);
    }
    else
        generate_instruction_from_str(nn->str);
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
    vector_foreach(code,i)
    {
        printf("instruction %03d ", i);        //instruction index
        print_instr(&ip_start[i]);             //instruction (op, and arg if there is one)
        printf("\n");
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


int define_var(symbol *sym)
{
	static char *var_addr = SIM_MEM + SIM_VARS_OFFSET;

    //get number of bytes
    symbol *type = sym->type;
    size_t bytes = type->tspec->bytes;

    //allocate the var
    if(sym->lifetime == STATIC)
    {
        sym->var = var_addr;
        var_addr += bytes;
        return 0;
    }
    else
    {
    	if(sym->is_argument)	//bp - offset
    	{
    		sym->var = (char*)local_arg_addr;
    		local_arg_addr -= bytes;
	    	return bytes;
	        
	    }
	    else	//local var (bp + offset)
	    {
	    	sym->var = (char*)local_var_addr;
	        local_var_addr += bytes;
	        return bytes;
	    }
    }
}


meta_op META_OPS[] =
{
    {"enter",   (char*[]){"pushv bp", "pushv sp", "pop bp", NULL}},   //push bp; bp = sp
    {"leave",   (char*[]){"pushv bp", "pop sp", "pop bp", NULL}},      //sp = bp; pop bp
    {"ret",     (char*[]){"jmp", NULL}}    //or "pop ip"
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

static bool filter_semact(void *n)
{
    pnode *nn = (pnode *)n;
	return (!(nn->is_nonterminal) && nn->ntype == SEMACT);
}