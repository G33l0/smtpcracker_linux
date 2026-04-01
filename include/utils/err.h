/* err.h */
#ifndef ERR_H
#define ERR_H

#define ERR_MAX 32

typedef struct {
    const char *file;
    int line;
    const char *func;
    int code;
    const char *reason;
} ERR_ENTRY;

void ERR_put_error(int code, const char *reason,
                   const char *file, int line, const char *func);

void ERR_print_errors(void);
void ERR_clear(void);

#define ERR(code, reason) \
    ERR_put_error(code, reason, __FILE__, __LINE__, __func__)

#endif
