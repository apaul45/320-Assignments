/*********************/
/* errmsg.c          */
/* for Par 3.20      */
/* Copyright 1993 by */
/* Adam M. Costello  */
/*********************/

/* This is ANSI C code. */
#ifdef __STDC_ALLOC_LIB__
#define __STDC_WANT_LIB_EXT2__ 1
#else
#define _POSIX_C_SOURCE 200809L
#endif

#include "errmsg.h"  /* Makes sure we're consistent with the declarations. */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static char* errorMessage = NULL;

void set_error(char *msg){
    //If not set to null, free before using strdup again
    //if (is_error()) clear_error();
    //If msg is null, then manualy set errorMessage to null instead of using strdup with a null pointer
    if (msg) errorMessage = strdup(msg);
    else errorMessage = NULL;
}

int is_error(){
    return (errorMessage == NULL ? 0 : 1);
}

int report_error(FILE *f){
    int s = 0;
    if (is_error()) s = fprintf(f, "%.163s", errorMessage);;
    return (s<0?s:0);
}

void clear_error(){
    free(errorMessage);
}
const char * const outofmem = "Out of memory.\n";
