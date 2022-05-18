/*********************/
/* errmsg.h          */
/* for Par 3.20      */
/* Copyright 1993 by */
/* Adam M. Costello  */
/*********************/

/* This is ANSI C code. */

/* Any function which uses errmsg must, before returning, */
/* either set errmsg[0] to '\0' (indicating success), or  */
/* write an error message string into errmsg, (indicating */
/* failure), being careful not to overrun the space.      */
#include <stdio.h>

extern const char * const outofmem;  /* "Out of memory.\n" */
/**
 * @brief 
 * Declare a static char pointer to be the error message string
 * Since it must be able hold any message, use strdup to copy a string into the pointer 
 * Thus, the pointer must be freed before the program exits to avoid memory leaks
 */
static char* errorMessage;

/**
 * @brief  Set an error indication, with a specified error message.
 * @param msg Pointer to the error message.  The string passed by the caller
 * will be copied.
 */
void set_error(char *msg);

/**
 * @brief  Test whether there is currently an error indication.
 * @return 1 if an error indication currently exists, 0 otherwise.
 */
int is_error();

/**
 * @brief  Issue any existing error message to the specified output stream.
 * @param file  Stream to which the error message is to be issued.  
 * @return 0 if either there was no existing error message, or else there
 * was an existing error message and it was successfully output.
 * Return non-zero if the attempt to output an existing error message
 * failed.
 */
int report_error(FILE *file);

/**
 * Clear any existing error indication and free storage occupied by
 * any existing error message.
 */
void clear_error();
