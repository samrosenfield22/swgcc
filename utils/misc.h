

#ifndef MISC_H_
#define MISC_H_

#define ARRAY_LEN(arr) (sizeof(arr)/sizeof(arr[0]))

//
int arr_search_str(char **arr, size_t len, const char *str);
char *firstchr(const char *str, const char *cs);

#endif //MISC_H_