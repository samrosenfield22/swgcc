/**/

#ifndef VECTOR_H_
#define VECTOR_H_

#include <stddef.h>
#include <stdbool.h>


#define vector(type, len)		vector_create_internal(sizeof(type), len)



//protos
void *vector_create_internal(size_t elem_size, size_t len);	//creates a new vector (user code should call vector(), not this)
void *vector_from_arr_internal(void *arr, size_t len, size_t elem_size);
void vector_destroy(void *v);							//deletes vector v
bool vector_resize(void *vptr, size_t new_len);			//resizes the vector (vptr is a double-ptr to it)
bool vector_inc(void *v);
bool vector_dec(void *v);
bool vector_insert(void *v, size_t index);
bool vector_delete(void *v, size_t index);
void *vector_copy(void *v);
void vector_intersect(void *vunion, void *a_only, void *b_only, void *a, void *b);
bool vector_swap(void *v, size_t a, size_t b);
int vector_search(void *v, int term);
int vector_search_str(char **v, const char *str);
void *vector_nth(void *v, size_t index);
size_t vector_len(void *v);								//returns the vector length (number of elements)
bool vector_is_empty(void *v);
size_t vector_internal_len(void *v);					//
size_t vector_elem_size(void *v);						//
size_t vector_total_size(void *v);						//

#define vector_last(v) v[vector_len(v)-1]
#define vector_append(v, item)	do {vector_inc(&v); vector_last(v) = item;} while(0)
#define vector_foreach(v, i)	for(int i=0; i<vector_len(v); i++)

//if we just pass {a,b,c,d...} the array gets interpreted as multiple args instead of a single arg, so we wrap it in parenthesis
#define ARRAY_LEN(a) (sizeof(a)/sizeof(a[0]))
#define vector_from_arr(...)  vector_from_arr_wrap( (__VA_ARGS__) )
#define vector_from_arr_wrap(arr) vector_from_arr_internal(arr, ARRAY_LEN(arr), sizeof(arr[0]))

//stack macros
#define vector_push		vector_append
#define vector_pop(v) 	({void *last = vector_last(v); vector_dec(&v); last;})


//map is an expression of the variable n
//ex. int *out = vector_map(in, n*n);   //square all elements
#define vector_map(v, map)                              \
  ({                                                    \
      typeof(*v) n;                                     \
      typeof(map) *vout = vector(*vout, vector_len(v)); \
      vector_foreach(v, _mindex) {                      \
          n = v[_mindex];                               \
          vout[_mindex] = map;                          \
      }                                                 \
      vout;                                             \
  })
//previously: typeof(map) *vout = (typeof(map) *)vector(typeof(map), vector_len(v));

//filter is an expression of the variable n
//ex. vector_filter(v, n>10 && n&0b1);
#define vector_filter(v, filter)                  		\
  ({                                              		\
      vector_foreach(v, _findex)                        \
      {                                           		\
          typeof(*v) n = v[_findex];                    \
          if(!(filter))                           		\
          {                                       		\
              vector_delete(&v, _findex);               \
              _findex--;                                \
          }                                       		\
      }                                           		\
  })

//fold is an expression of the variables l and r as the left and right nodes
//ex. sum = vector_fold(nums, l + r);
//max = vector_fold(nums, (l>r)? l : r);
#define vector_fold(v, fold)                      				\
  ({                                              				\
      typeof(*v) l = v[0], r;                     				\
      for(int _fdindex=1; _fdindex<vector_len(v); _fdindex++)   \
      {                                           				\
          r = v[_fdindex];                                      \
          l = fold;                               				\
      }                                           				\
      l;                                          				\
  })

//ex. vector_gen(int, 1, n++, 10);  //make a vector with numbers 1 to 10
#define vector_gen(type, first, incr, last)   	\
    ({                                          \
        type* v = vector(type, 0);              \
        for(type n=first; ; incr) {             \
            vector_append(v, n);                \
            if(n==last) break;                  \
        }                                       \
        v;                                      \
    })



/*





//the map must be an expression that uses the identifier "n" to represent each vector item
//ex. vector_map(nums, n*n, int)	//square each item

//int *b = vector_map(a, n*n, int);
#define vector_map(vec, map, type)										\
({																		\
	type* vout = NULL;													\
	if(sizeof(type) == vector_elem_size(vec)) {							\
		vout = vector_create_internal(sizeof(type), vector_len(vec));	\
		vector_foreach(vec, i) {										\
			type n = vec[i];											\
			vout[i] = map;												\
		}																\
	}																	\
	vout;																\
})

//vector_filter(n, func)
#define vector_filter(vec, filter)				\
({												\
	vector_foreach(vec, i) {					\
		typeof(*vec) n = vec[i];					\
		if(!(filter))							\
			vector_delete(&vec, i--);				\
	}											\
})
*/
#define vector_copy_filter(vec, filter)			\
({												\
	typeof(*vec) *vc = vector_copy(vec);		\
	vector_filter(vc, filter);					\
	vc;											\
})


#endif //VECTOR_H_
