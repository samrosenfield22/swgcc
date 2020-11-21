


#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "stack.h"

//don't call this directly, use the stack_create() macro function
void *stack_create_internal(int depth, size_t elem_size)
{
	stack *s = malloc(sizeof(*s));
	if(!s) return NULL;

	s->array = malloc(depth * sizeof(void *));
	if(!(s->array)) 
	{
		free(s);
		return NULL;
	}

	s->sp = s->array;
	s->elem_size = elem_size;

	return s;
}

void stack_delete(void *s)
{
	stack *st = (stack *)s;

	free(st->array);
	free(st);
}

void stack_clear(void *s)
{
	stack *st = (stack *)s;
	st->sp = st->array;
}

void stack_push(void *s, void *elem)
{
	stack *st = (stack *)s;

	*(st->sp) = malloc(st->elem_size);
	memcpy(*(st->sp), elem, st->elem_size);
	(st->sp)++;
}

//the popped element needs to be freed by the caller
void *stack_pop(void *s)
{
	stack *st = (stack *)s;

	if(st->sp == st->array)
		return NULL;

	(st->sp)--;
	return *(st->sp);
}

int stack_depth(void *s)
{
	stack *st = (stack *)s;
	int depth_bytes = (void*)(st->sp) - st->array;
	return (depth_bytes / sizeof(void*));
}

bool stack_is_empty(void *s)
{	
	stack *st = (stack *)s;
	return (st->sp == st->array);
}

void stack_dump(void *s)
{
	stack *st = (stack *)s;

	printf("dumping stack with %d elements\n", ((void*)st->sp - st->array)/sizeof(void*));
}