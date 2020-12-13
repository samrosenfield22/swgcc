

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "typesys.h"

/*typedef struct type_builder_s
{
	//the enum,
	int child_ct;
} type_builder;*/


typedef enum type_builder_e
{
	POINTER_TB,
	ARRAY_TB,
	STRUCT_TB,
	UNION_TB,
	FUNCTION_TB,
	NAME_TB,	//names for struct/union elements get their own node

	BASIC_TYPE_TB,
	NOT_A_TYPEBUILDER

} type_builder;

//extend the dag node type
struct typenode_s
{
	struct 	//must be first!
	{
		typenode **parents;
	    typenode **children;
	    char *str;
	};

	type_builder tb;
	size_t size;		//size of the type, in bytes
};

//
void make_basic(char *str, size_t size);
typenode *typenode_create(char *str, size_t size, type_builder tb);
static typenode *parse_tree_to_type(pnode *pt);
type_builder get_pnode_typebuilder(pnode *n);
symbol *get_pnode_basic_type(pnode *n);


/*typenode **basic_types;
void init_type_system(void)
{
	basic_types = vector(*basic_types, 0);

	//append the types (int, char, float...)
	make_basic("int", sizeof(long));
	make_basic("char", 1);
}

void make_basic(char *str, size_t size)
{

	typenode *t = typenode_create(str, size, BASIC_TYPE_TB);
	vector_append(basic_types, t);
}*/

typenode *typenode_create(char *str, size_t size, type_builder tb)
{
	printfcol(YELLOW_FONT, "\tcreating a typenode: %s\n", str);

	typenode *t = dag_create_extra(sizeof(*t), str);
	t->tb = tb;
	t->size = size;
	return t;
}

void resolve_type(symbol *sym, pnode *dcltor, char *type)
{
	/*printf("\n\n\ntype tree (base type is %s):\n\n", type);
	ptree_print(dcltor);
	getchar();

	int bt = -1;
	vector_foreach(basic_types, i)
	{
		printf("basic type %s\n", basic_types[i]->str);
		if(strcmp(basic_types[i]->str, type)==0)
		{
			bt = i; break;
		}
	}
	if(bt == -1)
	{
		printf("unrecognized type: \'%s\'\n", type);
		exit(-1);
	}*/

	//return;

	//add a new node containing the type to the bottom of the tree
	//nope, to the top
	pnode *tn = pnode_create(false, TERMINAL, type, NULL);
	tree_add_child(tn, dcltor);

	//something like
	sym->type = (symbol*)parse_tree_to_type(tn);
}

/*
     int *p[20];           int f(int a, char b)

        array                       function
       /     \                     /        \
      20     ptr               product      int
              |               /       \
             int            int       char

*/

/*
	get the type dag for the node

	if the node is a *
*/
static typenode *parse_tree_to_type(pnode *pt)
{
	//recursively search the types dag
	/*
	
	if pt describes a basic type
		assert(pt is a leaf node)
		return a ptr to that type in the basic_types list
	else if pt describes a composite type
		collect the type dags for all the children -- parse_tree_to_type(pt->children[i])
		if they have a common parent of type pt
			return the parent
		else make one, link them


		recurse to pt->children
			if that child has a matching (structurally equivalent) type dag

			if not,
				build it ()

	*/

	/*
		if it's a basic type, get that symbol
		it it's a ptr, get *(type so far)
		it it's an array, get (type so far)[]
	*/

	typenode **type_stack = vector(*type_stack, 0);
	pnode **specs = ptree_filter(pt, get_pnode_typebuilder(n) != NOT_A_TYPEBUILDER, true);
	assert(vector_len(specs));
	typenode *t;
	vector_foreach(specs, i)
	{
		type_builder tb = get_pnode_typebuilder(specs[i]);
		//printf("tb = %d (%d)\n", tb, NOT_A_TYPEBUILDER);
		switch(tb)
		{
			case BASIC_TYPE_TB:
				//printf("getting basic type %s\n", specs[i]->str);
				t = (typenode *)get_pnode_basic_type(specs[i]);
				assert(t);
				vector_push(type_stack, t);
				break;

			case POINTER_TB:
			case ARRAY_TB:
				//printf("adding composite type %s\n", specs[i]->str);
				t = vector_pop(type_stack);
				//int index = array_search(t->parents, vector_len(t->parents), get_pnode_typebuilder(n) == tb);
				int index = array_search(t->parents, vector_len(t->parents), ((typenode *)n)->tb == tb);
				typenode *parent;
				if(index == -1)
				{
					//make it
					char *str = (tb==POINTER_TB)? "*":"[]";
					size_t size = (tb==POINTER_TB)? sizeof(void*) : -1;
					parent = typenode_create(str, size, tb);
					dag_add_child(parent, t);
				}
				else
					parent = t->parents[index];

				vector_push(type_stack, parent);
				break;

			case FUNCTION_TB: assert(0); break;	//for now
			case NOT_A_TYPEBUILDER: break;

			default: assert(0);
		}
			
	}

	assert(vector_len(type_stack) == 1);
	typenode *type = vector_pop(type_stack);
	vector_destroy(type_stack);
	vector_destroy(specs);

	//printf("%s\n", type->str);

	//
	printf("printing type:\n");
	//tree_print_pretty(type, NULL);
	print_type(type);
	//getchar();
	return type;

}

/*typenode *get_matching_typedag_parent(pnode *pt, typenode *type)
{
	//for each parent of the typenode, check if 
	vector_foreach(type->parents, i)
	{
		//if(type->parents[i] matches the type described by pt)
			return type->parents[i];
	}
	return NULL;
}*/

//if the type is a composite type, returns the typebuilder for that type construction
type_builder get_pnode_typebuilder(pnode *n)
{
	if(n->ntype == SEMACT || n->ntype == IDENT)	return NOT_A_TYPEBUILDER;
	//else if(strlen(n->str)==0)					return NOT_A_TYPEBUILDER;

	else if(get_pnode_basic_type(n))					return BASIC_TYPE_TB;
	//else if(n->ntype==TERMINAL || n->ntype==SEMACT)	return NOT_A_TYPEBUILDER;

	else if(is_nonterm_type(n, "dclptr"))		return POINTER_TB;
	else if(is_nonterm_type(n, "dclarr"))		return ARRAY_TB;
	else if(is_nonterm_type(n, "funcdeflist"))	return FUNCTION_TB;

	//else assert(0);
	else return NOT_A_TYPEBUILDER;
}

//returns a pointer to the basic type symbol described by pn
symbol *get_pnode_basic_type(pnode *pn)
{
	//printf("searching for basic type: \'%s\'... ", pn->str);
	//printf("%s\n", symbol_search(pn->str, SYM_BASIC_TYPE)? "found":"nope");
	return symbol_search(pn->str, SYM_BASIC_TYPE);
}

void print_type(typenode *t)
{
	/*typenode **tflat = tree_filter(t, 1, true);
	vector_foreach(tflat, i)
	{
		switch(tflat[i]->tb)
		{
			case BASIC_TYPE_TB: printf("")
		}
	}*/

	tree_traverse(t, printf("%s", n->str), true);
}

