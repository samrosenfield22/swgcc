

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
	PRODUCT_TB,
	ARRAY_TB,

	BASIC_TYPE_TB

} type_builder;

//extend the dag node type
typedef struct typenode_s typenode;
struct typenode_s
{
	struct 	//must be first!
	{
		typenode **parents;
	    typenode **children;
	    char *str;
	};

	type_builder tb;
	size_t size;
};

//
void make_basic(char *str, size_t size);
type_builder get_pnode_typebuilder(pnode *n);
typenode *get_pnode_basic_type(pnode *n);


typenode **basic_types;
void init_type_system(void)
{
	basic_types = vector(*basic_types, 0);
	//append the types (int, char, float...)

	make_basic("int", 4);
	make_basic("char", 1);
}

void make_basic(char *str, size_t size)
{
	typenode *t = dag_create_extra(sizeof(*t), str);
	t->tb = BASIC_TYPE_TB;
	t->size = size;

	vector_append(basic_types, t);
}

void resolve_type(symbol *sym, pnode *dcltor, char *type)
{
	//printf("\n\n\n");
	//ptree_print(dcltor);
	//getchar();

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
	}
}
/*
     int *p[20];           int f(int a, char b)

        array                       function
       /     \                     /        \
      20     ptr               product      int
              |               /       \
             int            int       char

*/
typenode *parse_tree_to_type(pnode *pt)
{
	//recursively search the types dag
	/*
	
	if pt describes a basic type
		assert(pt is a leaf node)
		return a ptr to that type in the basic_types list
	else if pt describes a composite type
		collect the type dags for all the children -- parse_tree_to_type(pt->children[i])
		see if they have a common parent of type pt
		if not, make one, link them


		recurse to pt->children
			if that child has a matching (structurally equivalent) type dag

			if not,
				build it ()

	*/

	//see if the node describes a basic type or composite
	type_builder tb = get_pnode_typebuilder(pt);
	if(tb == BASIC_TYPE_TB)
	{
		return get_pnode_basic_type(pt);
	}
	else	//node describes a composite type
	{
		//collect the type dags for all the children
		//typenode *child_types = vector_map(pt->children, parse_tree_to_type(n));

		//find the common-est parent for the child types, 
	}

	//for now
	return NULL;
}

typenode *get_matching_typedag_parent(pnode *pt, typenode *type)
{
	//for each parent of the typenode, check if 
	vector_foreach(type->parents, i)
	{
		//if(type->parents[i] matches the type described by pt)
			return type->parents[i];
	}
	return NULL;
}

//if the type is a composite type, returns the typebuilder for that type construction
type_builder get_pnode_typebuilder(pnode *n)
{
	return 0;
}

//returns a pointer to the basic type described by n
typenode *get_pnode_basic_type(pnode *n)
{
	//assert(n is a leaf node);
	return NULL;
}

