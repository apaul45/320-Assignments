#include "hi.h" /* This is our headerfile */

/**
 * Here's the actual definition of our say_hi() function
 * as you can see it simply returns the string "Hi" Or does it??
 */

char* say_hi(){
    return "Hi";
}


float temperatureCalculator(float celsius, int* pointer){
    /* To avoid a fraction getting truncated, float is used over int here */
    float fahrenheit;
    fahrenheit = (9.0/5.0) * celsius + 32;

    float fahrenheitArray[5];
    fahrenheitArray[0] = fahrenheit;
    fahrenheitArray[1] = fahrenheit-20.0;

    /* To access entries in an array using pointers, a variable called fahrenheitArray (ie, the name of the array)
    can be used-- this is a pointer containing the address to the first entry in the array-- the first space 
    allocated for an array 
    
    Since fahrenheitArray contains the address to the first entry, later entries can be accessed by adding their index to it
    */
    float firstEntry = *fahrenheitArray;
    float secondEntry = *(fahrenheitArray+1);

    /* Change the value of the pointTO variable in main(): recently added*/
    *pointer = *pointer + 20;

    return secondEntry;
}
/* Back over to main to finish our program */
