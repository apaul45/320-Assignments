
#include "hi.h" /* This is our headerfile, containing the definition for say_hi() */
/**
 * This is the full definition of a main function in C the beginning of our
 * program
 * it is the equivalent of public static void main in java
 *
 * In C main always returns an int, it represents the exit status of your
 * program
 * a program that runs successfully returns 0 one that has an error returns any
 * positive value
 *
 * The arguments passed to main are as follows:
 * - int argc: this is the size of the argv array
 * - char const *argv[] is an array of strings that are the command line
 *   arguments for the program
 * - char const *envp[] are the environment variables availible to any program
 *   run in the OS's environment
 * You can define main to have any of these arguments removed `int main()` for
 * example is entirely valid.
 */

/* In order to use functions in main or other functions, they must be declared (function prototypes)/defined before said functions */
void* noop(void* v){return NULL;}

int main(int argc, char const *argv[], char const *envp[]) {
    /*
     * say_hi() is a function that returns a character pointer (a.k.a a string)
     * Journey on over to `src/hi.c` for the next comments.
     */
    char *hello = say_hi();

    /*
     * This is a print statement in C it places the arguments
     * following the format string into a string following the
     * format string's design.
     */
    printf("%s, %s%c\n", hello, "World", '!');
    printf("HI HI \n");
    printf("LOL ");
    printf("LMAO \n");
    printf("LMFAO \n");
    printf("address of var: %p\n", &hello);

    /* In order to print different data types, the type has to be specified
    in the first argument of printf 
    
    For ex: To print a int followed by a float, the first arg would be "%d %f"
    Escape characters can also be added in between, before, and after characters as needed
    */

    /* One big purpose of pointers is the ability to directly manipulate variables :
    even though c is call by value, pointers can be passed into a function. The variable that is 
    being pointed to can then be changed by deferencing the pointer in the function

    Lines 58-61, and line 82 were recently added and do not appear in the console. 
    */
    int pointTo = 50;
    printf("print john %d",pointTo);

    printf("print pointto");

    float fahrenheit = temperatureCalculator(32.0, (int*)&pointTo);

    //Print initial value of pointTo
    printf("Initial value of pointTo: %d\n", pointTo);
    printf("%f\t%f\n", fahrenheit, 32.0);


    /* In order to access pointers (which are variables containing the addresses to other variables), 
    '&' must be concatenated with the variable in question-- to declare a pointer, & must be declared after the type
    
    Then to access the value of the variable using the pointer, '*' must be concatenated in front of the pointer 
    */
    float* fahrenheitPointer = &fahrenheit;

    printf("Address of fahrenheit variable: %p\n", fahrenheitPointer);
    printf("Value in fahrenheit-- using the pointer: %f\n", *fahrenheitPointer);
    printf("%f\n", *&fahrenheit);

    //Print the value of pointTo, which should now be differenent: newly added and does not appear in console
    printf("%d\n", pointTo);

    /* End of our program */
    return 0;
}

/* Head over to tests/test.c to see what a unit test looks like */
