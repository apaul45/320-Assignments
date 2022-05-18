#include <stdlib.h>

#include "argo.h"
#include "global.h"
#include "debug.h"
#include "validity.h"

//Use stdbool to be able to declare and use boolean variables
#include <stdbool.h> 


int argLengthParser(char* argument);

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 */

int validargs(int argc, char **argv) {
    debug("Entered valid args\n");
    // TO BE IMPLEMENTED

    //This variable can be used to deterrmine which return code to return
    //Set the initial value of successOrFail to the condition of too few variables
    //argc being = to 1 means there are no arguments (since program name counts as a arg)
    bool successOrFail = !(argc == 1);

    //Set initial value of global options to 0
    global_options = 0;
    
    //If too many args and the first arg isnt -h, then -1 is returned and usage is called w it in main
    //If -h is the first arg, then EXIT_SUCCESS 
    if (argc > 4){
        //The first actual argument is in argv[1] as an array of chars (which is what a string is in C), aka *argv+1
        //Then the string itself can be retrieved by going through the array of chars representing it (w pointers)
        char* arg1 = *(argv+1);
        char char1 = *arg1;
        char char2 = *(arg1+1);

        //If the first argument is -h, then set the bool to true so
        //that the EXIT_SUCCESS code is returned (and vice versa if it isnt)
        successOrFail = (argLengthParser(arg1) == 2 && char1=='-' && char2=='h');

        //Also set global_options to have a msb of 1 if the first arg is -h, and 0x0 if invalid
        global_options = (successOrFail ? HELP_OPTION : 0);
    }
    else{
        debug("%d\n", argc);
        debug("Reached else statement\n");
        //Use successOrFail to verify if a second argument exists before setting it to argv+1
        char* arg1 = successOrFail ? *(argv+1) : *argv;
        char char1 = *arg1;
        char char2 = *(arg1+1);
        int arg1Length = argLengthParser(arg1);

        if (arg1Length>2) return -1;

        if (char1=='-' && char2=='h'){
            global_options = HELP_OPTION;   
            return EXIT_SUCCESS;
        }
        else if(char1=='-' && char2=='c'){
            switch(argc){
                //If -c is the only argument, then perform the actions for it
                case 2:{
                    debug("Reached -c case\n");
                    //Set global_options to canolical mode and return
                    global_options = CANONICALIZE_OPTION;
                    successOrFail = true;
                    break;
                }
                //If there are 2 args, check if the second one is -p. If not, then the argument is invalid
                case 3:{
                    debug("Reached -c -p case\n");
                    char* arg2 = *(argv+2);
                    //If the second arg is not -p, then invalid arg
                    if (argLengthParser(arg2)>2 || *arg2 != '-' || *(arg2+1) != 'p') return -1;
                    //Since -p is also mentioned, the 3rd msb is 1 (for -c), and the fourth msb is 1 (for -p). So the most sig. byte is 3
                    //lsb is 4, as this is the default value if a indent isn't specified
                    global_options = 0x30000004;
                    successOrFail = true;
                    break;
                }
                //If there are 4 args, check if the second is -p and if the third is a non-negative int. If not, the argument is invalid
                case 4:{
                    debug("Reached -c -p indent case\n");
                    char* arg2 = *(argv+2);
                    //If the second arg is not -p, then invalid arg
                    if (argLengthParser(arg2)>2 || *arg2 != '-' || *(arg2+1) != 'p') return -1;
                    //if the third arg is not a number, then invalid arg
                    int indent = numParser(*(argv+3));
                    if (indent==-1) return -1; 
                    //Most significant byte is equal to 3 (see case 3). Least significant byte is equal to indent
                    global_options = 0x30000000 + indent;
                    successOrFail = true;
                    break;
                }
                //Unlike -v below, a default case is not needed because too few/too many cases has already been checked for, and every other case for -c is above
            };
        }
        else if (char1=='-' && char2=='v'){
            switch(argc){
                //If -v is the only argument, perform the actions for it before returning EXIT_SUCCESS
                case 2:{
                    debug("Reached -v case\n");
                    //Set global options such that its second msb is 1
                    global_options = VALIDATE_OPTION;
                    successOrFail = true;
                    break;
                }
                //If there are more than 2 args, then the default case will be triggered since too many/too few args has already
                //been checked. Hence, it will return -1 since it is invalid
                default: {
                    successOrFail=false;
                    break;
                }
            }
        }
        //If the first argument isn't -h, -c, or -v, then the args are invalid
        else return -1;
    }
    debug("Reached the end of valid args\n");
    return (successOrFail ? EXIT_SUCCESS : -1);
}

//Return the length of a command line argument
int argLengthParser(char* argument){
    int counter=0;
    //A newline marks the end of a command line argument, and has an ascii value of 0 
    while(*argument != 0){
        counter++;
        ++argument;
    }
    //A newline character marks the end of a command line argument
    return counter;
}
//Parses a int argument (if possible), returns a negative int otherwise
int numParser(char* arg1){
    int num = 0;
    int i = argLengthParser(arg1);
    //In order to get the right digit, have to subtract by 48 (since 49 is ascii value for '1')
    for (i-=1; i>=0; i--){
        //Verify that the next digit is valid
        //Can check if a char is numeric using lower ('1' = ascii val of 49) and upper bounds(9- ascii val of 57)
        if (!(argo_is_digit(*arg1))) return -1; //indent cant be negative, or a decimal
        int digit = ((int)*arg1)-48;
        int j = i, place=1;
        //Nested loop to get the proper place 
        while(j>0){
            place *= 10;
            j--;
        }
        num+=(place*digit);
        ++arg1;
    }
    if (num>255) return -1; //indent cant be >255
    return num;
}

