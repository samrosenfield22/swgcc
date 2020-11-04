/**/

#ifndef VECTOR_H_
#define VECTOR_H_

#include <stddef.h>
#include <stdbool.h>

//#include "../../utils.h"

//uncomment to use embedded malloc implementation
//#include "../embedded_malloc/emalloc.h"

/*
//create a vector, passing it a compound array literal or an initialized array, ex.
//int *v = vector(int, (int[]){2,3,5,7,11});
//char array[] = "hello"; char *v = vector(char, array);
#define vector(type, ...)                                                                   \
		vector_create_internal(sizeof(type), sizeof(__VA_ARGS__)/sizeof(type), __VA_ARGS__);
*/

//if using a platform which does not support variadic macros, use this instead -- you must
//create an empty vector then call vector_resize, but /shrug
//#define vector(type)
//		vector_create_internal(sizeof(type), 0, NULL)

//#define vector(type)			vector_create_internal(sizeof(type))
#define vector(type, len)		vector_create_internal(sizeof(type), len)

#define vector_dump(v, fmt)		vector_dump_internal(v, fmt, #v)

/*

int n[8];
int *n = vector(int); vector_resize(&n, 8);

int n[] = {5,6,4,3};
int *n = vector(int); 

*/

//protos
//void *vector_create_internal(size_t elem_size, size_t len, void *init);	//creates a new vector (user code should call vector(), not this)
void *vector_create_internal(size_t elem_size, size_t len);	//creates a new vector (user code should call vector(), not this)
void vector_destroy(void *v);							//deletes vector v
bool vector_resize(void *vptr, size_t new_len);			//resizes the vector (vptr is a double-ptr to it)
bool vector_inc(void *v);
bool vector_dec(void *v);
bool vector_insert(void *v, size_t index);
bool vector_delete(void *v, size_t index);
bool vector_swap(void *v, size_t a, size_t b);
void *vector_nth(void *v, size_t index);
//void *vector_last(void *v);
size_t vector_len(void *v);								//returns the vector length (number of elements)
size_t vector_internal_len(void *v);					//
size_t vector_elem_size(void *v);						//
size_t vector_total_size(void *v);						//

#define vector_last(v) v[vector_len(v)-1]

void vector_dump_internal(void *v, const char *fmt, const char *name);


#endif //VECTOR_H_
