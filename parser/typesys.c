

#include "typesys.h"

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
};

typenode *basic_types;
void init_basic_types(void)
{
	basic_types = vector(*basic_types, 0);
	//append the types (int, char, float...)
}

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
	typebuilder tb = get_pnode_typebuilder(pt);
	if(tb == BASIC_TYPE_TB)
	{
		return get_pnode_basic_type(pt);
	}
	else	//node describes a composite type
	{
		//collect the type dags for all the children
		//typenode *typechildren = vector(*typechildren, 0);
		typenode *child_types = vector_map(pt->children, parse_tree_to_type(n));

		//find the common-est parent for the child types, 
	}
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

}

//returns a pointer to the basic type described by n
typenode *get_pnode_basic_type(pnode *n)
{
	//assert(n is a leaf node);
}

