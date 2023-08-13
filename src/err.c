#include <stdio.h>
#include "../include/err.h"

int neo_err;

char *n_strerror(int n_err){
    switch(n_err){
        case NEO_ERR_NULL:
            return "Received NULL pointer";
        default:
            return "Success";
    }
    return NULL;
}