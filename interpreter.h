

#ifndef INTERPRETER_H_
#define INTERPRETER_H_


#include <stdbool.h>

#define VERBOSE true
#define SILENT false

typedef enum fail_type
{
    PASS,
    LEX_FAIL,
    PARSE_FAIL,
    SEMANTIC_FAIL,
} fail_type;


void launch_interpreter(bool verbose);
fail_type interpreter(int *result, bool verbose, char *code);

#endif //INTERPRETER_H_