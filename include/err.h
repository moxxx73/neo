#ifndef NEO_ERR_H
#define NEO_ERR_H

#include <errno.h>

enum{
    NEO_ERR_SUCESS,
    NEO_ERR_NULL
};

extern int neo_err;

char *n_strerror(int n_err);

#endif