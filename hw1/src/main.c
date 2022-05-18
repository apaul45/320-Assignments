#include <stdio.h>
#include <stdlib.h>

#include "argo.h"
#include "global.h"
#include "debug.h"
#include "validity.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/* 
    Command line arguments get passed into main(). 
    argc is the argument count, which includes the name of the 
    program itself (aka argo). argv is a a 2d array, where each 
    element in the array is an array of chars (aka the argument as a string)

    Since array syntax is not allowed, pointers can be used to access the 
    arguments instead (see validargs())
*/
int main(int argc, char **argv)
{
    //Display usage with the proper return code from validargs()
    int returnCode = validargs(argc, argv);
    debug("%x\n", global_options);
    switch(global_options){
        case HELP_OPTION:
        case 0: {
            //If -h was passed or global options is 0 (meaning the arguments were invalid),
            //then display usage. The appropriate return code will then be printed at the end of main()
            USAGE(*argv, returnCode);
            break;
        }
        //If - v is invoked, then read data from standard input
        //(stdin) and validate that it is syntactically correct JSON. 
        case VALIDATE_OPTION:{
            debug("Reached -v case in main\n");
            ARGO_VALUE* new_json = argo_read_value(stdin);
            if (new_json == NULL) returnCode = -1;
            break;
        }
        //default will handle the case of pretty print
        case CANONICALIZE_OPTION:
        default: {
            debug("reached -c case in main\n");
            level = 0; //Reset level before proceeding
            ARGO_VALUE* new_json = argo_read_value(stdin);
            if (new_json == NULL) return -1;
            argo_write_value(new_json, stdout);
            break;
        }
    }

    //if valid_args returns -1, then main() should return EXIT_FAILURE
    if (returnCode == -1) return EXIT_FAILURE;
    return returnCode;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
