#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "stores.h"

int store_next = 0;
static Variable *data_store = NULL;
static int store_size = MAX_DATA;
static int search_for_var(char* var);

/*
 * This is the "data store" module for Mush.
 * It maintains a mapping from variable names to values.
 * The values of variables are stored as strings.
 * However, the module provides functions for setting and retrieving
 * the value of a variable as an integer.  Setting a variable to
 * an integer value causes the value of the variable to be set to
 * a string representation of that integer.  Retrieving the value of
 * a variable as an integer is possible if the current value of the
 * variable is the string representation of an integer.
 */

static int search_for_var(char* var){
    if (!data_store) data_store = malloc(MAX_DATA * sizeof(Variable));
    int i;
    for (i=0; i<store_next; i++){
        if (strcmp(data_store[i].name, var) == 0) return i;
    }
    return -1;
}

int clear_data_store(){
    int i;
    for (i = 0; i<store_next; i++){
        free(data_store[i].name);
        free(data_store[i].value);
    }
    free(data_store);
    return 0;
}

/**
 * @brief  Get the current value of a variable as a string.
 * @details  This function retrieves the current value of a variable
 * as a string.  If the variable has no value, then NULL is returned.
 * Any string returned remains "owned" by the data store module;
 * the caller should not attempt to free the string or to use it
 * after any subsequent call that would modify the value of the variable
 * whose value was retrieved.  If the caller needs to use the string for
 * an indefinite period, a copy should be made immediately.
 *
 * @param  var  The variable whose value is to be retrieved.
 * @return  A string that is the current value of the variable, if any,
 * otherwise NULL.
 */
char *store_get_string(char *var) {
    int index = search_for_var(var);
    return (index > -1 ? data_store[index].value : NULL);
}

/**
 * @brief  Get the current value of a variable as an integer.
 * @details  This retrieves the current value of a variable and
 * attempts to interpret it as an integer.  If this is possible,
 * then the integer value is stored at the pointer provided by
 * the caller.
 *
 * @param  var  The variable whose value is to be retrieved.
 * @param  valp  Pointer at which the returned value is to be stored.
 * @return  If the specified variable has no value or the value
 * cannot be interpreted as an integer, then -1 is returned,
 * otherwise 0 is returned.
 */
int store_get_int(char *var, long *valp) {
    int old_errno = (int)errno; //Make sure to save the old value of errno
    errno = 0; //Set the value of errno to 0 before calling the parse function
    int index = search_for_var(var);
    if (index != -1){
        char *endptr;
        long val = strtol(data_store[index].value, &endptr, 10);
        if (errno != 0){
            errno = old_errno;
            return -1;
        }
        *valp = val;
        return 0;
    }
    return -1;
}

/**
 * @brief  Set the value of a variable as a string.
 * @details  This function sets the current value of a specified
 * variable to be a specified string.  If the variable already
 * has a value, then that value is replaced.  If the specified
 * value is NULL, then any existing value of the variable is removed
 * and the variable becomes un-set.  Ownership of the variable and
 * the value strings is not transferred to the data store module as
 * a result of this call; the data store module makes such copies of
 * these strings as it may require.
 *
 * @param  var  The variable whose value is to be set.
 * @param  val  The value to set, or NULL if the variable is to become
 * un-set.
 */
int store_set_string(char *var, char *val) {
    int index = search_for_var(var);

    if (index == -1){
        Variable new_var;
        new_var.name = malloc(strlen(var));
        new_var.value = malloc(strlen(val));
        strcpy(new_var.name, var);
        strcpy(new_var.value, val);

        if (store_next == store_size){
            store_size = store_next + MAX_DATA;
            data_store = realloc(data_store, store_size * sizeof(Variable));
        } 
        data_store[store_next] = new_var;
        store_next++;
    }
    else{
        //If specified value is NULL, remove any existing value & unset the var
        if (!val) data_store[index].value = '\0';
        //Otherwise, replace the existing value with the new one
        else{
            Variable var = data_store[index];
            if (strlen(val) > strlen(var.value)){
                var.value = realloc(var.value, strlen(val));
            }
            strcpy(var.value, val);
        }
    }
    return 0;
}

/**
 * @brief  Set the value of a variable as an integer.
 * @details  This function sets the current value of a specified
 * variable to be a specified integer.  If the variable already
 * has a value, then that value is replaced.  Ownership of the variable
 * string is not transferred to the data store module as a result of
 * this call; the data store module makes such copies of this string
 * as it may require.
 *
 * @param  var  The variable whose value is to be set.
 * @param  val  The value to set.
 */
int store_set_int(char *var, long val) {
    char* ptr; size_t sizec;
    FILE* stream = open_memstream(&ptr, &sizec);
    fprintf(stream, "%ld", val);
    fclose(stream);

    //After converting the value to a string, call set string to insert
    int status = store_set_string(var, ptr);
    free(ptr);
    return status;
}

/**
 * @brief  Print the current contents of the data store.
 * @details  This function prints the current contents of the data store
 * to the specified output stream.  The format is not specified; this
 * function is intended to be used for debugging purposes.
 *
 * @param f  The stream to which the store contents are to be printed.
 */
void store_show(FILE *f) {
    if (!data_store) data_store = malloc(MAX_DATA * sizeof(Variable));
    int i; 
    for (i = 0; i<store_next; i++){
        fprintf(f, "%s = %s\n", data_store[i].name, data_store[i].value);
    }
}
