/**/

#ifndef STACK_H_
#define STACK_H_

#include <stdbool.h>

#include <stdio.h>

#define stack_create(depth, type) stack_create_internal(depth, sizeof(type))

typedef struct stack_s
{
	void **sp;
	int elem_size;
	void *array;
} stack;



void *stack_create_internal(int depth, size_t elem_size);
void stack_delete(void *s);
void stack_clear(void *s);
void stack_push(void *s, void *elem);
void *stack_pop(void *s);
int stack_depth(void *s);
bool stack_is_empty(void *s);

void stack_dump(void *s);

#endif //STACK_H_