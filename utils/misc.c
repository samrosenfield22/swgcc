

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "misc.h"

/*

*/
int arr_search_str(char **arr, size_t len, const char *str)
{
	for(int i=0; i<len; i++)
	{
		if(strcmp(arr[i], str)==0)
			return i;
	}
	return -1;
}

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

