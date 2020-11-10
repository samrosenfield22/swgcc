


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "vector.h"

#define CHECK(condition, on_fail)	do {assert(condition); if(!(condition)) on_fail;} while(0)

//header metadata for vector
typedef struct vector_hdr_s
{
	size_t len, alloc_len, elem_size;
} vector_hdr;

//protos for statics
static inline vector_hdr *vec_header(void *v);
static size_t next_power_of_two(size_t n);


//void *vector_create_internal(size_t elem_size, size_t len, void *init)
void *vector_create_internal(size_t elem_size, size_t len)
{
	//alloc the vector
	size_t alloc_len = next_power_of_two(len);
	vector_hdr *v = malloc(sizeof(*v) + alloc_len*elem_size);
	CHECK(v != NULL, return NULL);

	//initialize header
	v->len = len;
	v->alloc_len = alloc_len;
	v->elem_size = elem_size;

	//
	return (v+1);
}

void vector_destroy(void *v)
{
	CHECK(v, return);
	free(vec_header(v));
}

//returns true if the resize succeeded, false if it failed
//call w vector_resize(&array, new_len)
bool vector_resize(void *vptr, size_t new_len)
{
	CHECK(vptr, return false);
	void **vdptr = vptr;
	vector_hdr *vhdr = vec_header(*vdptr);

	//size_t new_alloc_len = next_power_of_two(new_len);

	//check if vector needs to reallocate
	if((new_len <= vhdr->alloc_len) && new_len > (vhdr->alloc_len)>>1)
	//ex. if the alloc_len is 8, anything >4 and <=8 means we don't need to reallocate
	//if(new_alloc_len == vhdr->alloc_len)
	{
		vhdr->len = new_len;
		return true;
	}
	else
	{
		size_t new_alloc_len = next_power_of_two(new_len);
		size_t new_alloc_size = (new_alloc_len * vhdr->elem_size) + sizeof(vector_hdr);
		vector_hdr *new_v = realloc(vhdr, new_alloc_size);

		if(new_v == NULL)
			return false;
		else
		{
			*vdptr = new_v+1;
			new_v->len = new_len;
			new_v->alloc_len = new_alloc_len;
			return true;
		}
	}
}

bool vector_inc(void *v)
{
	CHECK(v, return false);
	size_t len = vec_header(*(void**)v)->len;
	return vector_resize(v, len+1);
}

bool vector_dec(void *v)
{
	CHECK(v, return false);
	size_t len = vec_header(*(void**)v)->len;
	return len? vector_resize(v, len-1) : false;
}

bool vector_insert(void *v, size_t index)
{
	CHECK(v, return false);
	CHECK(index < vector_len(v), return false);

	if(!vector_inc(&v))
		return false;

	//
	void *copy_from = v + vector_elem_size(v)*index;
	void *copy_to = copy_from + vector_elem_size(v);
	size_t copy_size = (vector_len(v)-index-1) * vector_elem_size(v);
	memmove(copy_to, copy_from, copy_size);

	return true;
}

bool vector_delete(void *vv, size_t index)
{
	void *v = *(void **)vv;
	CHECK(v, return false);
	CHECK(index < vector_len(v), return false);

	//
	void *copy_to = v + vector_elem_size(v)*index;
	void *copy_from = copy_to + vector_elem_size(v);
	size_t copy_size = (vector_len(v)-index-1) * vector_elem_size(v);
	memmove(copy_to, copy_from, copy_size);

	if(!vector_dec(vv))
		return false;

	return true;
}

void *vector_copy(void *v)
{
	vector_hdr *vhdr = vec_header(v);
	void *copy = vector_create_internal(vhdr->elem_size, vhdr->len);
	memcpy(copy, v, vhdr->elem_size * vhdr->len);
	return copy;
}

//int *anotb, *bnota, *aandb;
//vector_intersect(&aandb, &anotb, &bnota, a, b);
void vector_intersect(void *isect, void *a_only, void *b_only, void *a, void *b)
{
	if(vector_elem_size(a) != vector_elem_size(b))
		return;
	void *ao = vector_copy(a);
	void *bo = vector_copy(b);
	void *isc = vector_create_internal(vector_elem_size(a), 0);

	for(int ai=0; ai<vector_len(ao); ai++)
	{
		void *a_item = vector_nth(ao, ai);
		int bi = vector_search(bo, *(int*)a_item);
		if(bi != -1)
		{
			//printf("--- match! a(%d)=%d, b(%d)=%d\n", ai, *(int*)a_item, bi, *(int*)vector_nth(bo, bi));
			
			//append the item to the intersect vector
			vector_inc(&isc);
			//printf("intersect len is now %d\n", vector_len(isc));
			*(int*)vector_nth(isc, vector_len(isc)-1) = *(int*)a_item;
			/*for(int i=0; i<vector_len(isc); i++)
				printf("%d ", *(int*)vector_nth(isc, i));
			printf("\n");*/

			vector_delete(&ao, ai);
			vector_delete(&bo, bi);
			ai--; bi--;
		}
	}

	if(a_only) *(void**)a_only = ao;	else vector_destroy(ao);
	if(b_only) *(void**)b_only = bo;	else vector_destroy(bo);
	if(isect) *(void**)isect = isc;		else vector_destroy(isc);
} 

bool vector_swap(void *v, size_t a, size_t b)
{
	CHECK(v, return false);
	CHECK(a < vector_len(v), return false);
	CHECK(b < vector_len(v), return false);

	if(a == b) return true;

	void *temp = malloc(vector_elem_size(v));
	CHECK(temp, return false);

	void *ap = vector_nth(v, a);
	void *bp = vector_nth(v, b);
	memcpy(temp, ap, vector_elem_size(v));
	memcpy(ap, bp, vector_elem_size(v));
	memcpy(bp, temp, vector_elem_size(v));

	free(temp);
	return true;
}

int vector_search(void *v, int term)
{
	//printf("\tsearching v of len %d\n", vector_len(v));
	for(int i=0; i<vector_len(v); i++)
	{
		//printf("\tcomparing %d,%d\n", *(int*)vector_nth(v,i), term);
		if(*(int*)vector_nth(v,i) == term)
			return i;
	}
	return -1;
}

//usually using native array indexing (thing[n]) is a better choice
void *vector_nth(void *v, size_t index)
{
	CHECK(v, return NULL);
	return v + index*(vector_elem_size(v));
}

size_t vector_len(void *v)
{
	CHECK(v, return 0);
	return vec_header(v)->len;
}

size_t vector_internal_len(void *v)
{
	CHECK(v, return 0);
	return vec_header(v)->alloc_len;
}

size_t vector_elem_size(void *v)
{
	CHECK(v, return 0);
	return vec_header(v)->elem_size;
}

size_t vector_total_size(void *v)
{
	CHECK(v, return 0);
	vector_hdr *vhdr = vec_header(v);
	size_t arrsize = (vhdr->alloc_len)*(vhdr->elem_size);
	return (sizeof(vector_hdr) + arrsize);
}

void vector_dump_internal(void *v, const char *fmt, const char *name)
{
	CHECK(v, return);
	vector_hdr *vhdr = vec_header(v);

	printf("> \"%s\": %d elems (%d alloc'd) of size %d. total bytes: %d\n",
		name, vhdr->len, vhdr->alloc_len, vhdr->elem_size, vector_total_size(v));
	for(int i=0; i<vhdr->len; i++)
	{
		printf("\t%s[%d]:\t", name, i);
		printf(fmt, *(int *)(v+i*vhdr->elem_size));
		putchar('\n');
	}
}

static inline vector_hdr *vec_header(void *v)
{
	vector_hdr *vcast = v;
	return vcast-1;
}

//this definitely isn't the best way
static size_t next_power_of_two(size_t n)
{
	for(size_t i=1; i; i<<=1)
	{
		if(i >= n)
			return i;
	}

	//if n is bigger than the max(size_t)/2, set it to max(size_t)
	return SIZE_MAX;
}
