#include <stdio.h>
#include <utils/err.h>

static ERR_ENTRY err_stack[ERR_MAX];
static int err_top = 0;

void ERR_put_error(int code, const char *reason,
                   const char *file, int line, const char *func)
{
    if (err_top >= ERR_MAX)
        return;

    err_stack[err_top].code   = code;
    err_stack[err_top].reason = reason;
    err_stack[err_top].file   = file;
    err_stack[err_top].line   = line;
    err_stack[err_top].func   = func;
    err_top++;
}

void ERR_print_errors(void)
{
    for (int i = err_top - 1; i >= 0; i--) {
        fprintf(stderr,
            "Error:%d:%s:%s:%d:%s\n",
            err_stack[i].code,
            err_stack[i].func,
            err_stack[i].file,
            err_stack[i].line,
            err_stack[i].reason);
    }
}

void ERR_clear(void)
{
    err_top = 0;
}
