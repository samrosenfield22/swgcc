

#ifndef MISC_H_
#define MISC_H_

#define ARRAY_LEN(arr) (sizeof(arr)/sizeof(arr[0]))
#define array_foreach(arr, i)	for(int i=0; i<ARRAY_LEN(arr); i++)

/* expr is an expression where "n" represents each array element, i.e.
int index = array_search(numbers, is_prime(n));		//returns the index of the 1st prime element, or
-1 if no matching terms are found
*/
#define array_search(arr, len, expr) ({			\
	int index = -1;								\
	typeof(arr[0]) n;							\
	for(int i_=0; i_<len; i_++) {				\
		memcpy(&n, &arr[i_], sizeof(arr[0]));	\
		if(expr) {								\
			index = i_;							\
			break;								\
		}										\
	}											\
	index;										\
})

#define array_search_str(arr, len, str)	array_search(arr, len, strcmp(n, str)==0)

#define sort_array(arr, compar)	sort(arr, ARRAY_LEN(arr), sizeof(arr[0]), compar)
#define def_stable_compar(func, size)					\
	int stable_##func(const void *a, const void *b)		\
	{													\
		int comparison = func(a,b);						\
		if(comparison) return comparison;				\
		else											\
		{												\
			int a_ord = *(int*)(a+size);				\
			int b_ord = *(int*)(b+size);				\
			return a_ord - b_ord;						\
		}												\
	}

//
//int arr_search_str(char **arr, size_t len, const char *str);
char *firstchr(const char *str, const char *cs);

void sort(void *base, size_t nitems, size_t size, int (*compar)(const void *, const void*));

#endif //MISC_H_