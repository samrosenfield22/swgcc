

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "misc.h"

/*

*/
/*int arr_search_str(char **arr, size_t len, const char *str)
{
	for(int i=0; i<len; i++)
	{
		if(strcmp(arr[i], str)==0)
			return i;
	}
	return -1;
}*/

/*
like strchr, but for multiple chars -- returns a pointer to the earliest-found match in str
if no match is found, returns a pointer to the string's nul-terminator
*/
char *firstchr(const char *str, const char *cs)
{
	const char *first = str + strlen(str);

	for(const char *c=cs; *c; c++)
	{
		char *match = strchr(str, *c);
		if(match && match<first)
		{
			if(match == str)	return match;
			first = match;
		}
	}

	return (char*)first;
}


/* to generate a comparison function:
	write your comparison function, as you would (let's call it "compar")
	use the "def_stable_compar" macro:
		def_stable_compar(compar, sizeof(thing))
	this expands to declare a function whose name is the passed function, prepended with "stable_"
	i.e. stable_compar
	then pass this to sort!
	sort(base, nitems, size, stable_compar);

	i know, i know, it's horrible and ugly.
*/
/*void sort(void *base, size_t nitems, size_t size, int (*compar)(const void *, const void*))
{
	//add the order to each item
	typedef struct extend_s
	{
		char item[size];	//the original item gets copied here
		size_t order;
	} extend;
	extend *ext_base = malloc(nitems * sizeof(*ext_base));
	assert(ext_base);

	for(int i=0; i<nitems; i++)
	{
		void *item = (char*)base + i*size;
		memcpy(ext_base[i].item, item, size);
		ext_base[i].order = i;
	}

	//sort. the sorting function must take order into account (see the def_stable_compar() macro)
	qsort(ext_base, nitems, sizeof(extend), compar);

	//copy back to the original array
	for(int i=0; i<nitems; i++)
	{
		void *item = (char*)base + i*size;
		memcpy(item, ext_base[i].item, size);
	}
}*/

