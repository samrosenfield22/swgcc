/**/

#include <stdlib.h>
#include <assert.h>

#include "stack.h"

int main(void)
{
	void *s = stack_create(256, int);
	assert(s);
	stack_dump(s);
	printf("%s\n", stack_is_empty(s)? "empty" : "nonempty");

	int dummy;

	dummy = 5; stack_push(s, &dummy); printf("%s\n", stack_is_empty(s)? "empty" : "nonempty"); //stack_dump(s);
	dummy = 6; stack_push(s, &dummy);
	dummy = 4; stack_push(s, &dummy);
	dummy = 3; stack_push(s, &dummy);

	stack_dump(s);

	int *p;
	p = stack_pop(s); printf("%d\n", *p); free(p);
	p = stack_pop(s); printf("%d\n", *p); free(p);
	p = stack_pop(s); printf("%d\n", *p); free(p); printf("%s\n", stack_is_empty(s)? "empty" : "nonempty");
	p = stack_pop(s); printf("%d\n", *p); free(p); printf("%s\n", stack_is_empty(s)? "empty" : "nonempty");
	

	return 0;
}