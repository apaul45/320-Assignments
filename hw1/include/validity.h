//This header file will be used to make the numParser function available to all source files
#include <stdbool.h>
#include "argo.h"
#ifndef VALIDITY_H
#define VALIDITY_H

#include <stdio.h>

int numParser(char* arg1);
double reverseNum(long num);
double parseExp(ARGO_NUMBER* n, int* expPointer, FILE *f);
//level is used to keep track of which level a argo value 
//is in the data structure representing a json file
//A level of 0 or 1 indicates that the argo value is on the highest level
int level;

// //Use invalidChar to determine whether to keep advancing or break out of 
// //the parsing process to return null (and print to stderr)
// bool invalidChar = false;

//Use is_first_exp to determine if e appears for the first time or not in read_number
#define is_first_exp(b, c) ((b)>0 && (c)==0)
//Use is_close_comma to determine if a , } or ] appears
#define is_close_comma(c) ((c) == ',' || (c) == '}' || (c) == ']')
//Use is_dot_exp_neg to determine if this char is one of those 3 chars
#define is_dot_exp_neg(c) ((c) == '.' || (c) == '-' || (c) == 'e')
#define is_lowercase_hex(c) ((c) >= 'a' && (c) <= 'f')
//Declare function prototypes here
//This will be used to detect if pretty print is enabled. If so, then it'll print a 
//newline ot the output stream along with necessary indentation
void pretty_newline_detector(FILE *f);
void write_float(double value, FILE *f);
void parseUnicode(ARGO_STRING* n, FILE *f);
bool isUnicode(FILE *f, int count);
int argo_read_array(ARGO_ARRAY *n, FILE *f);
int argo_read_objectArray(ARGO_VALUE *n, FILE *f);
int argo_read_basic(char basic, ARGO_BASIC *n, FILE *f);
#endif