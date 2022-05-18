#include <stdlib.h>
#include <stdio.h>
#include "validity.h"
#include "argo.h"
#include "global.h"
#include "debug.h"
//Use stdbool to be able to declare and use boolean variables
#include <stdbool.h> 

/**
 * @brief  Read JSON input from a specified input stream, parse it,
 * and return a data structure representing the corresponding value.
 * @details  This function reads a sequence of 8-bit bytes from
 * a specified input stream and attempts to parse it as a JSON value,
 * according to the JSON syntax standard.  If the input can be
 * successfully parsed, then a pointer to a data structure representing
 * the corresponding value is returned.  See the assignment handout for
 * information on the JSON syntax standard and how parsing can be
 * accomplished.  As discussed in the assignment handout, the returned
 * pointer must be to one of the elements of the argo_value_storage
 * array that is defined in the const.h header file.
 * In case of an error (these include failure of the input to conform
 * to the JSON standard, premature EOF on the input stream, as well as
 * other I/O errors), a one-line error message is output to standard error
 * and a NULL pointer value is returned.
 *
 * @param f  Input stream from which JSON is to be read.
 * @return  A valid pointer if the operation is completely successful,
 * NULL if there is any error.
 */
ARGO_VALUE *argo_read_value(FILE *f) {
    argo_lines_read++;
    bool invalidChar = false;
    ARGO_VALUE newArg;
    newArg.name.content = ARGO_NULL; //name is null unless value is a member
    //Add the newValue to the argo_values storage array before proceeding
    *(argo_value_storage+argo_next_value) = newArg;
    ARGO_VALUE* newValue = argo_value_storage+argo_next_value;
    argo_next_value++;
    char first = fgetc(f); 
    while(!invalidChar && first != EOF){
        argo_chars_read++;
        if(argo_is_whitespace(first)){
            debug("Whitespace reached\n");
            //If a  newline is reached, increment argo_lines_read and reset argo_chars_read
            if (first == ARGO_LF) {argo_lines_read++; argo_chars_read = 0;}
        }
        else if (argo_is_digit(first) || first == ARGO_MINUS){
            debug("Number reached\n");
            // Create and allocate a argo value for a number type, then let argo_read_number create the necessary argo_number;
            newValue->type = ARGO_NUMBER_TYPE;
            // unget this digit so that it can be parsed in the argo read number function
            ungetc(first, f);
            if (argo_read_number(&newValue->content.number, f) == -1) invalidChar = true;
            break;
        }
        else if (first == ARGO_LBRACE || first == ARGO_LBRACK){
            debug("Object or array reached\n");
            //Create and allocate a argo value for a object type, then let argo_read_objectArray create the necessary argo_object
            newValue->type = (first == ARGO_LBRACE ? ARGO_OBJECT_TYPE : ARGO_ARRAY_TYPE);
            //Initialize the member or element list as a dummy node
            ARGO_VALUE h; 
            h.type = ARGO_NO_TYPE;
            *(argo_value_storage+argo_next_value) = h;
            if (first == ARGO_LBRACE) newValue->content.object.member_list = argo_value_storage + argo_next_value;
            else newValue->content.array.element_list = argo_value_storage + argo_next_value;
            argo_next_value++;
            if (argo_read_objectArray(newValue, f) == -1) invalidChar = true;
            break;
        }
        else if (first == ARGO_QUOTE){ 
            debug("String reached\n");
            newValue->type = ARGO_STRING_TYPE;
            ungetc(ARGO_QUOTE, f);
            if (argo_read_string(&newValue->content.string, f) == -1) invalidChar = true;
            break;
        }
        else if (first == 'n' || first == 't' || first == 'f'){
            debug("Basic reached\n");
            //Create and allocate a argo value for a basic type, then let argo_read_basic create the necessary argo_basic
            newValue->type = ARGO_BASIC_TYPE;
            if (argo_read_basic(first, &newValue->content.basic, f) == -1) invalidChar = true;
            break;
        }
        else{
            fprintf(f, "Error: Invalid char at line %d", argo_lines_read);
            invalidChar = true;
            break;
        }
        if (!invalidChar) first = fgetc(f);
    }
    //If a invalid char was found, then print a specific message to stderr before returning a null pointer;
    if (invalidChar) return NULL;
    return newValue;
}

/**
 * @brief  Read JSON input from a specified input stream, attempt to
 * parse it as a JSON string literal, and return a data structure
 * representing the corresponding string.
 * @details  This function reads a sequence of 8-bit bytes from
 * a specified input stream and attempts to parse it as a JSON string
 * literal, according to the JSON syntax standard.  If the input can be
 * successfully parsed, then a pointer to a data structure representing
 * the corresponding value is returned.
 * In case of an error (these include failure of the input to conform
 * to the JSON standard, premature EOF on the input stream, as well as
 * other I/O errors), a one-line error message is output to standard error
 * and a NULL pointer value is returned.
 *
 * @param f  Input stream from which JSON is to be read.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_read_string(ARGO_STRING *s, FILE *f) {
    debug("Reached start of argo read string\n");
    if (fgetc(f) != ARGO_QUOTE){
        fprintf(f, "Error: Not a valid string\n");
        return -1;
    }
    //Reset the length and capacity fields
    s->capacity = 0;
    s->length = 0;
    char nextChar = fgetc(f);
    //Loop until the end of the file is reached, or a end quote is found
    while(nextChar != ARGO_QUOTE){
        if (nextChar == EOF){
            fprintf(stderr, "Error: A closing quote for a string was not found on line %d\n.", argo_lines_read);
            return -1;
        }
        if (nextChar == ARGO_BSLASH){
            char after = fgetc(f);
            switch (after){
                case ARGO_T: argo_append_char(s, (ARGO_CHAR)ARGO_HT);break;
                case ARGO_R: argo_append_char(s, (ARGO_CHAR)ARGO_CR);break;
                case ARGO_QUOTE: argo_append_char(s, (ARGO_CHAR)ARGO_QUOTE);break;
                case ARGO_BSLASH: argo_append_char(s, (ARGO_CHAR)ARGO_BSLASH);break;
                case ARGO_F: argo_append_char(s, (ARGO_CHAR)ARGO_FF); break;
                case ARGO_B: argo_append_char(s, (ARGO_CHAR)8); break;
                case ARGO_N: argo_append_char(s, (ARGO_CHAR)ARGO_LF); break;
                //If u, then check if four hex digits follow 
                case ARGO_U: {
                    parseUnicode(s,f);
                    break;
                }
                //If none of these, then add the backslash and the next char to content, and update i and nextChar 
                default: {
                    argo_append_char(s, nextChar);
                    argo_append_char(s, after);
                    break;
                }
            }
        }
        //If a newline is found (ie, the ascii 10, not "\n"), then print error and return -1
        else if (nextChar == ARGO_LF){
            debug("Reached newline error\n");
            argo_lines_read++; 
            fprintf(stderr, "Error: Newline found in member on line %d\n", argo_lines_read);
            return -1;
        }
        else argo_append_char(s, nextChar);
        debug("In argo string: %c\n", nextChar);
        nextChar = fgetc(f);
    }
    debug("String succesfully parsed\n");
    return 0;
}

void parseUnicode(ARGO_STRING* n, FILE *f){
    int i;
    bool isValid = isUnicode(f, 1);
    debug("%d\n", isValid);
    if (isValid){
        int number = 0;
        for (i = 3; i>=0; i--){
            char nextChar = fgetc(f);
            //Get the power of 16 that this parsed char should be multiplied by
            int power = 1; 
            int j = i;
            while (j>0){
                power *= 16;
                j--;
            }
            if (is_lowercase_hex(nextChar)) number+=((nextChar-87)*power);
            else if (argo_is_digit(nextChar)) number+=((nextChar-48)*power);
            else number+=((nextChar-55)*power);
        }
        debug("Unicode value as hex digit: %x\n", number);
        argo_append_char(n, number);
    }
    else{
        for (i=0; i<4; i++){
            argo_append_char(n, fgetc(f));
        }
    }
}
bool isUnicode(FILE *f, int count){
    char nextChar = fgetc(f);
    debug("Count: %d, Char: %c\n", count, nextChar);
    if (count == 4){
        ungetc(nextChar, f);
        return argo_is_hex(nextChar);
    }
    else{
        if(argo_is_hex(nextChar)){
            bool isValid = isUnicode(f, ++count);
            ungetc(nextChar, f);
            return isValid;
        } 
        else{ungetc(nextChar, f); return false;}
    }
}
/**
 * @brief  Read JSON input from a specified input stream, attempt to
 * parse it as a JSON number, and return a data structure representing
 * the corresponding number.
 * @details  This function reads a sequence of 8-bit bytes from
 * a specified input stream and attempts to parse it as a JSON numeric
 * literal, according to the JSON syntax standard.  If the input can be
 * successfully parsed, then a pointer to a data structure representing
 * the corresponding value is returned.  The returned value must contain
 * (1) a string consisting of the actual sequence of characters read from
 * the input stream; (2) a floating point representation of the corresponding
 * value; and (3) an integer representation of the corresponding value,
 * in case the input literal did not contain any fraction or exponent parts.
 * In case of an error (these include failure of the input to conform
 * to the JSON standard, premature EOF on the input stream, as well as
 * other I/O errors), a one-line error message is output to standard error
 * and a NULL pointer value is returned.
 *
 * @param f  Input stream from which JSON is to be read.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_read_number(ARGO_NUMBER *n, FILE *f){
    int digitCounter = 0, dotIndex = 0, expIndex = 0; //Use dotIndex and expIndex to keep track of where a "." or exponent appears in this number, if any
    bool charReached = false; //Use charReachedto break out of the parsing loop if whitespace/a closing bracket/comma is reached
    char firstDigit = fgetc(f);
    long int num = 0;
    bool isNeg = false;
    //Reset length and capacity
    n->string_value.capacity = 0;
    n->string_value.length = 0;

    while (firstDigit != EOF && !charReached){
        debug("Digit in this iteration of the read number loop: %c\n", firstDigit);
        debug("Digit counter in this iteration: %d\n Exp index: %d\n Dot index: %d\n", digitCounter, expIndex, dotIndex);
        argo_chars_read++;
        if (argo_is_digit(firstDigit)){
            if (num == (long)0 && digitCounter > 0 && dotIndex == 0){
                fprintf(stderr, "Error: Numbers with leading zeros are not allowed\n");
                return -1;
            }
            argo_append_char(&n->string_value, firstDigit);
            int digit = firstDigit-48;
            num = (num*10) + digit;
        }
        else if (digitCounter == 0  && firstDigit == ARGO_MINUS) {
            isNeg = true; 
            argo_append_char(&n->string_value, firstDigit); 
            firstDigit = fgetc(f); 
            continue;
        }
        //If a "." is found, use dotIndex to note the iteration it was found in (and allow for errors if multiple . appear)
        //A valid dot is one that appears once following one or more digits
        else if (dotIndex == 0 && firstDigit == ARGO_PERIOD) {dotIndex = digitCounter; 
            argo_append_char(&n->string_value, firstDigit); 
            firstDigit = fgetc(f); 
            continue;
        }
        else if (argo_is_whitespace(firstDigit)) {
            //If a newline is reached, increment lines read and reset chars read 
            if (firstDigit == ARGO_LF) {argo_lines_read++; argo_chars_read = 0;}
            //Break out of the loop if 1)whitespace reached and 2) one or more digits have been parsed already
            if (digitCounter>0) {charReached = true;  break;} else {firstDigit = fgetc(f); continue;}
        }
        //If an exponent is reached, break out of the loop and note its index
        //A valid exponent is one that appears once following one or more digits, and that doesn't immediately follow a "."
        else if (is_first_exp(digitCounter, expIndex) && (dotIndex == 0 || dotIndex != digitCounter-1) && argo_is_exponent(firstDigit)){
            debug("Reached exp\n");
            argo_append_char(&n->string_value, firstDigit); 
            expIndex = digitCounter;
            break;
        }
        //If comma, }, or ] reached and one or more digits have been parsed, break out of the loop (using charReached), and unget
        else if (is_close_comma(firstDigit) && digitCounter>0){
            charReached = true;
            ungetc(firstDigit, f);
            break;
        }
        //If invalid char, break out of the loop (conditions below will make sure a error is noted)
        else break;

        firstDigit = fgetc(f);
        digitCounter++;
    }
    //If the loop terminated without stopping at whitespace/comma/}/], print an error and return null
    if (!charReached && expIndex==0){
        fprintf(stderr, "Error at line %d\n", argo_lines_read);
        return -1;  
    }
    else{
        debug("Number before parsing: %ld\n", num);
        //If a period or exponent appears, the number is only a valid float
        if (dotIndex != 0 || expIndex != 0) {n->valid_float = 1; n->valid_int = 0;} else {n->valid_int = 1; n->valid_float = 1;}
        n->valid_string = 1;
        int exp = 0;
        if (expIndex != 0) {if(parseExp(n, &exp, f) == -1) return -1;}
        if (dotIndex != 0) exp-=(digitCounter - dotIndex);
        debug("Value of exp: %d\n", exp);
        double number = (isNeg ? -1 * num : num);
        while(exp != 0){
            debug("Value of number in loop: %f\n", number);
            exp < 0 ? (number/=10.0) : (number*=10);
            exp<0 ? exp++ : exp--;
        }
        if (n->valid_float != 0) n->float_value = number; 
        if (n->valid_int != 0) n->int_value = number;
        debug("Number after parsing: %f\n", number);
        return 0;
    }
}
double parseExp(ARGO_NUMBER* n, int* expPointer, FILE *f){
    char nextChar = fgetc(f);
    bool invalid = false, isNeg = false;
    int digitCounter = 0, tenths = 1;
    long int num = 0;
    while(nextChar != EOF || !invalid){
        if (digitCounter == 0 && nextChar == ARGO_MINUS){
            argo_append_char(&n->string_value, nextChar);
            isNeg = true;
            nextChar = fgetc(f);
            continue;
        }
        else if (argo_is_digit(nextChar)){
            argo_append_char(&n->string_value, nextChar); 
            int digit = nextChar-48;
            num = (num*10) + (digit*tenths);
            tenths*=10;
        }
        else if (argo_is_whitespace(nextChar)) {
            //If a newline is reached, increment lines read and reset chars read 
            if (nextChar == ARGO_LF) {argo_lines_read++; argo_chars_read = 0;}
            //Break out of the loop if 1)whitespace reached and 2) one or more digits have been parsed already
            if (digitCounter>0) break; else {nextChar = fgetc(f); continue;}
        }
        //If comma, }, or ] reached and one or more digits read, break out of the loop (using charReached), and unget
        else if (is_close_comma(nextChar) && digitCounter > 0){
            ungetc(nextChar, f);
            break;
        }
        else {invalid = true; break;}

        nextChar = fgetc(f);
        digitCounter++;
    }
    if (invalid || nextChar == EOF) {fprintf(stderr, "Error parsing exponent at line %d\n", argo_lines_read); return -1;}
    *expPointer = isNeg ? -1*num : num;
    return 0;
}

int argo_read_objectArray(ARGO_VALUE *n, FILE *f){
    debug("Object read function reached\n");
    bool member; //Use to indicate what to search for (in order of member, then value, then next and repeat)
    if (n->type == ARGO_OBJECT_TYPE) member = true; //member is only used for objects-- contains the logic to parse a member's name
    else member = false;
    bool success = false; //success is used to end the parsing when a closing } or ] is found
    bool value = true, next = true;
    ARGO_VALUE* head = n->type == ARGO_OBJECT_TYPE ? n->content.object.member_list:n->content.array.element_list;
    ARGO_VALUE* sentinel = head;
    head->next = head; head->prev = head; //Initialize the member or element list before parsing
    char nextChar = fgetc(f);
    while(nextChar != EOF && !success){
        //If this argo value is an object, create and add a argo_value to the argo_value array, and update its name
        if (member){
            debug("Member reached\n");
            ARGO_VALUE newVal; 
            //Parse the file until a quote appears: print & return error if any invalid chars found
            while(nextChar != ARGO_QUOTE){
                if (argo_is_whitespace(nextChar)){
                    //If a newline is reached, increment lines read and reset chars read 
                    if (nextChar == ARGO_LF) {argo_lines_read++; argo_chars_read = 0;}
                }
                else if (nextChar == ARGO_RBRACE && head == sentinel) return 0;//If closing brace reached with no member found, set next and prev and return 0
                else {fprintf(stderr, "Error: Next member not found on line %d\n", argo_lines_read); return -1;}
                debug("%c\n", nextChar);
                nextChar = fgetc(f);
            }
            debug("%c\n", nextChar);
            ungetc(nextChar, f);
            //Once a quote is found, allow argo_read_string to create a member
            if (argo_read_string(&(newVal.name), f) == -1) return -1;
            //Add the argo val to the array using the provided counter (dont increment this as itll be needed later)
            *(argo_value_storage + argo_next_value) = newVal;
            //Set member to false and value to true
            member = false; 
            value = true; 
            nextChar = fgetc(f);
        }
        //If value, then repeat what is done in the main argo_read_value, but first looking for a : (if object)
        //value is used for both arrays and objects
        else if (value){
            debug("Value reached\n");
            //If this value is an object, then it'll have already been added to the argo value array due to its name being parsed
            //If not, then a new argo value must be initialized and added to storage
            ARGO_VALUE* newValue;
            if (n->type == ARGO_OBJECT_TYPE) newValue = argo_value_storage + argo_next_value;
            else{
                ARGO_VALUE j; 
                j.name.content = ARGO_NULL;
                *(argo_value_storage + argo_next_value) = j;
                newValue = argo_value_storage + argo_next_value;
            }
            ++argo_next_value; //If object, then increment the next counter to the next open spot after retrieving the value. If array, increment after adding in & retrieving newValue

            if (n->type == ARGO_OBJECT_TYPE){
                debug("Reached the start of colon parsing. First char is: %c\n", nextChar);
                while(nextChar != ARGO_COLON){
                    //If whitespace, continue looking for a colon
                    if (argo_is_whitespace(nextChar)){
                        //If a  newline is reached, increment argo_lines_read and reset argo_chars_read
                        if (nextChar == ARGO_LF) {argo_lines_read++; argo_chars_read = 0;}
                    }
                    //If an invalid char (or EOF reached) is found, print an error and return
                    else if (nextChar != ARGO_COLON){
                        fprintf(f, "Error: ':' not found for member on line %d", argo_lines_read);
                        return -1;
                    }
                    nextChar = fgetc(f);
                }
                nextChar = fgetc(f); //if a colon was succesfully found, then advance to the next char to start searching for a value
            }
            while (nextChar != EOF){
                debug("Char in this iteration: %c\n", nextChar);
                if(argo_is_whitespace(nextChar)){
                    //If a  newline is reached, increment argo_lines_read and reset argo_chars_read
                    if (nextChar == ARGO_LF) {argo_lines_read++; argo_chars_read = 0;}
                }
                else if (argo_is_digit(nextChar) || nextChar == ARGO_MINUS){
                    debug("Argo read number reached\n");
                    newValue->type = ARGO_NUMBER_TYPE;
                    // unget this digit so that it can be parsed in the argo read number function
                    ungetc(nextChar, f);
                    if (argo_read_number(&newValue->content.number, f) != -1) value = false;  //Set value to false to signal that this is a valid value
                    break;
                }
                else if (nextChar == ARGO_LBRACE || nextChar == ARGO_LBRACK){
                    newValue->type = (nextChar == ARGO_LBRACE ? ARGO_OBJECT_TYPE : ARGO_ARRAY_TYPE);
                    ARGO_VALUE g;
                    g.type = ARGO_NO_TYPE;
                    *(argo_value_storage + argo_next_value) = g;
                    //Make sure to initialize the sentinel of the member or element list with a dummy node
                    if (nextChar == ARGO_LBRACE) newValue->content.object.member_list = argo_value_storage + argo_next_value;
                    else newValue->content.array.element_list = argo_value_storage + argo_next_value;
                    argo_next_value++;
                    //Create and allocate a argo value for a object type, then let argo_read_objectArray create the necessary argo_object
                    if (argo_read_objectArray(newValue, f) != -1) value = false;  //Set value to false to break out of the loop and signal that this is a valid value
                    break;
               }
                else if (nextChar == ARGO_QUOTE){ 
                    newValue->type = ARGO_STRING_TYPE;
                    ungetc(ARGO_QUOTE, f);
                    if (argo_read_string(&newValue->content.string, f) != -1) value = false;
                    break;
                }
                else if (nextChar == 'n' || nextChar == 't' || nextChar == 'f'){
                    debug("Reached basic type\n");
                    //Create and allocate a argo value for a basic type, then let argo_read_basic create the necessary argo_basic
                    newValue->type = ARGO_BASIC_TYPE;
                    if (argo_read_basic(nextChar, &newValue->content.basic, f) != -1) value = false;
                    break;
                }
                else if (n->type == ARGO_ARRAY_TYPE && nextChar == ARGO_RBRACK && head==sentinel) return 0; //Case for empty array
                else {fprintf(stderr, "Error: Invalid character found on line %d\n", argo_lines_read); break;}
                nextChar = fgetc(f); //If a value wasn't found yet, continue parsing the file in search for one 
            }
            //If the end of the file was reached without finding a value or an invalid value was found, then print and return error
            if (nextChar == EOF || value){
                if (nextChar == EOF) fprintf(stderr, "Error: Reached end of file while parsing.\n");
                return -1;
            }
            debug("Value successfully parsed\n");
            //If a value was succesfully parsed and added, then add this value to the member list, set next to true and increment argo_next_value
            head->next = newValue;
            head->next->prev = head;
            head = head->next;
            next = true;
        }
        //If next, search for a comma or closing brace
        //If comma, then set member to true
        //If bracket reached, then proceed to putting the argo values in this object's member list
        else if (next){
            debug("Next reached\n");
            nextChar = fgetc(f);
            char close = n->type == ARGO_OBJECT_TYPE ? ARGO_RBRACE : ARGO_RBRACK;
            while(nextChar != EOF && !success){
                debug("Char in this iteration of next: %c\n", nextChar);
                if(argo_is_whitespace(nextChar)){
                    //If a  newline is reached, increment argo_lines_read and reset argo_chars_read
                    if (nextChar == ARGO_LF) {argo_lines_read++; argo_chars_read = 0;}
                    nextChar = fgetc(f);
                }
                else if (nextChar == ARGO_COMMA){
                    //If a comma is found, set member to true for an object or value to true for array and break out of the loop
                    if (n->type == ARGO_OBJECT_TYPE) member = true;
                    else value=true;
                    nextChar = fgetc(f);
                    break;
                }
                else if (nextChar == close) success = true;
                else{
                    fprintf(stderr,"Error: No ',' or '}' found\n");
                    return -1;
                }
            }
            //If the end of the file was reached, print and return error
            if (nextChar == EOF){
                fprintf(stderr,"Error: No ',' or '}' found\n");
                return -1;
            }
        }
    }
    //Once done looping through the object, link the tail's next back to the sentinel, and the sentinel's prev to the tail
    sentinel->prev = head;
    head->next = sentinel;
    return 0;
}

int argo_read_basic(char basic, ARGO_BASIC *n, FILE *f){
    bool isInvalid = false;
    char second = fgetc(f);
    char third = fgetc(f);
    char fourth = fgetc(f);
    char fifth = fgetc(f); //Use fifth to make sure that this value is valid

    if (second == ARGO_LF) {argo_lines_read++; argo_chars_read=0;}
    if (third == ARGO_LF) {argo_lines_read++; argo_chars_read=0;}
    if (fourth == ARGO_LF) {argo_lines_read++; argo_chars_read=0;}
    if (fifth == ARGO_LF) {argo_lines_read++; argo_chars_read=0;}
    switch(basic){
        case (ARGO_T):{ 
            //If the fifth char is a comma, closing bracket, or whitespace, then this is a valid basic type
            if (second == 'r' && third == 'u' && fourth=='e' && (is_close_comma(fifth) || argo_is_whitespace(fifth))){
                *n = ARGO_TRUE;
                ungetc(fifth, f);
            }
            else isInvalid = true;
            break;
        }
        case(ARGO_F):{
            char sixth = fgetc(f);
            if (sixth == ARGO_LF) {argo_lines_read++; argo_chars_read=0;}
            if (second == 'a' && third == 'l' && fourth=='s' && fifth == 'e'&& (is_close_comma(sixth) || argo_is_whitespace(sixth))){
                *n = ARGO_FALSE;
                ungetc(sixth, f);
            }
            else isInvalid = true;
            break;
        }
        case('n'):{
            if (second == 'u' && third == 'l' && fourth=='l' && (is_close_comma(fifth) || argo_is_whitespace(fifth))){
                *n = ARGO_NULL;
                ungetc(fifth, f);
            } 
            else isInvalid = true;
            break;
        }
    }
    if (isInvalid){
        fprintf(f, "Error: Invalid char at line %d\n", argo_lines_read);
        return -1;
    }
    return 0;
}

/**
 * @brief  Write canonical JSON representing a specified value to
 * a specified output stream.
 * @details  Write canonical JSON representing a specified value
 * to specified output stream.  See the assignment document for a
 * detailed discussion of the data structure and what is meant by
 * canonical JSON.
 *
 * @param v  Data structure representing a value.
 * @param f  Output stream to which JSON is to be written.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_write_value(ARGO_VALUE *v, FILE *f) {
    //If name is not null, print out the name using argo_write_string
    if (v->name.content != ARGO_NULL){
        argo_write_string(&(v->name), f);
        //If pretty print is specified, then print a single space following the ':'
        global_options >= 0x30000000 ? fprintf(f, "%s", ": ") : fprintf(f, "%c", ':');
    }

    //Make sure to set and disable isTopLevel as a 
    switch(v->type){
        case (ARGO_NO_TYPE): break;
        //If a argo value is on the top level (ie, level=0) & pretty print is enabled, then print out a 
        //single newline after the value is printed along w/the required indentation
        //This will be handled by pretty_newline_detector
        case (ARGO_BASIC_TYPE):{
            ARGO_BASIC r = v->content.basic;
            fprintf(f, "%s", (r == ARGO_NULL ? ARGO_NULL_TOKEN : r == ARGO_TRUE ? ARGO_TRUE_TOKEN : ARGO_FALSE_TOKEN));
            if (level == 0) pretty_newline_detector(f);
            break;
        }
        case(ARGO_NUMBER_TYPE): {
            argo_write_number(&(v->content.number), f);
            if (level == 0) pretty_newline_detector(f);
            break;
        }
        case(ARGO_STRING_TYPE): {
            argo_write_string(&(v->content.string), f);
            if (level == 0) pretty_newline_detector(f);
            break;
        }
        case ARGO_OBJECT_TYPE: 
        case ARGO_ARRAY_TYPE: {
            //If level > 0, that means this array or object is nested, meaning 
            //Increment the level when an object or array reached
            level++;
            char open = (v-> type == ARGO_OBJECT_TYPE ? '{' : '[');
            char close = (v-> type == ARGO_OBJECT_TYPE ? '}' : ']');
            //If pretty print is specified, then print a single newline after a opening bracket
            //pretty_newline_detector will handle this
            fprintf(f, "%c", open);
            pretty_newline_detector(f);

            ARGO_VALUE* sentinel= (v->type == ARGO_ARRAY_TYPE ?
                                   v->content.array.element_list
                                  :v->content.object.member_list);
            
            //Get the actual head of the circular linked list 
            ARGO_VALUE* head = sentinel->next;

            //Recursively traverse the object, and terminate when the sentinel is reached by comparing the addresses
            while (head != sentinel){
                argo_write_value(head, f);
                head = head->next;
                //If this isn't the last element in the array or object, then add a comma
                if (head != sentinel){
                    fprintf(f, "%c", ARGO_COMMA);
                    pretty_newline_detector(f);
                }
            }
            //Decrease the top level after this object/array's members have been printed
            //This is so that the closing { or ] is at the previous indentation level
            level--;
            //If pretty print is enabled, then a newline should be printed right after the last member 
            //is printed: pretty_newline_detector will handle this
            pretty_newline_detector(f);
            fprintf(f, "%c", close); 
            break;
        }
    }
    if (level == 0 && global_options >= (CANONICALIZE_OPTION + PRETTY_PRINT_OPTION)) fprintf(f, "\n");//Make sure a newline is printed at the end of the object when pretty print is enabled
    return 0;
}

/**
 * @brief  Write canonical JSON representing a specified string
 * to a specified output stream.
 * @details  Write canonical JSON representing a specified string
 * to specified output stream.  See the assignment document for a
 * detailed discussion of the data structure and what is meant by
 * canonical JSON.  The argument string may contain any sequence of
 * Unicode code points and the output is a JSON string literal,
 * represented using only 8-bit bytes.  Therefore, any Unicode code
 * with a value greater than or equal to U+00FF cannot appear directly
 * in the output and must be represented by an escape sequence.
 * There are other requirements on the use of escape sequences;
 * see the assignment handout for details.
 *
 * @param v  Data structure representing a string (a sequence of
 * Unicode code points).
 * @param f  Output stream to which JSON is to be written.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_write_string(ARGO_STRING *s, FILE *f) {
    int length = s->length;
    ARGO_CHAR* text = s->content;
    int i =0;
    fprintf(f, "\""); //Make sure to start and end the string in quotations
    while(i<length){
       //Have to loop through elements to check what each one is:
       //Each element that is printed out must be greater than unicode 
       //control U+001F (aka 31 in dec), and less than U+O0FF (which is 195 191 (aka 0xFF) in dec)
       // '\' (dec 92) and " (dec 34) must be printed as escape seq (see switch case below)
       int c = *(text+i);
       if (c<0xFF){
           //Finally, check if this char is an escape sequence such as '\n'
           //If it is, print it as '\n' rather than as a newline, for example
           switch(c){
               //Newline
               case(10): fprintf(f, "%c%c", ARGO_BSLASH, ARGO_N); break;
               //Backspace
               case(8): fprintf(f, "%c%c", ARGO_BSLASH, ARGO_B); break;
               //Horz tab
               case(ARGO_HT): fprintf(f, "%c%c", ARGO_BSLASH, ARGO_T); break;
               //Form feed
               case(ARGO_FF): fprintf(f, "%c%c", ARGO_BSLASH, ARGO_F); break;
               //Carriage Return
               case(ARGO_CR): fprintf(f, "%c%c", ARGO_BSLASH, ARGO_R); break;
               //Double Quote
               case(ARGO_QUOTE): fprintf(f, "%c%c", ARGO_BSLASH, ARGO_QUOTE); break;
               //Back slash
               case(ARGO_BSLASH): fprintf(f, "%c%c", ARGO_BSLASH, ARGO_BSLASH); break;
               //If the char is less than 1f and not one of the escapes above, print it using \u
               default: {
                   if (c<31){
                        if (c<=16) fprintf(f, "%c%c%c%c%c%x", ARGO_BSLASH, ARGO_U, ARGO_DIGIT0, ARGO_DIGIT0, ARGO_DIGIT0, c);
                        else fprintf(f, "%c%c%c%c%x", ARGO_BSLASH, ARGO_U, ARGO_DIGIT0, ARGO_DIGIT0, c);
                   }
                   else fprintf(f, "%c", c); 
                   break;
               }
           }
       }
       //If the char is greater than or equal to 0x00FF, it has to be printed in unicode format (ie, '\u', followed by 0s (as needed) and the char's value in hex)
       else{
            //A number can be printed in hexadecimal format by using %x (for lowercase) or %X (uppercase) 
            //Make sure to check how many hex digits are in the char, so that the appropriate amounts of 0s can be printed out
            if (c==0xff) fprintf(f, "%c%c%c%c%x", ARGO_BSLASH, ARGO_U, ARGO_DIGIT0, ARGO_DIGIT0, c); //If 2 digits, print out 2 0s
            else if (c <= 0xfff) fprintf(f, "%c%c%c%x", ARGO_BSLASH, ARGO_U, ARGO_DIGIT0, c); //If c has 3 digits, then pad/hardcode in 1 zero
            else fprintf(f, "%c%c%x", ARGO_BSLASH, ARGO_U, c); //If c has 4 digits, then no 0s need to be manually printed
       }
       i++;
    }
    fprintf(f, "\""); //Make sure to start and end the string in quotations
    //Return 0 if success
    return 0;
}

/**
 * @brief  Write canonical JSON representing a specified number
 * to a specified output stream.
 * @details  Write canonical JSON representing a specified number
 * to specified output stream.  See the assignment document for a
 * detailed discussion of the data structure and what is meant by
 * canonical JSON.  The argument number may contain representations
 * of the number as any or all of: string conforming to the
 * specification for a JSON number (but not necessarily canonical),
 * integer value, or floating point value.  This function should
 * be able to work properly regardless of which subset of these
 * representations is present.
 *
 * @param v  Data structure representing a number.
 * @param f  Output stream to which JSON is to be written.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_write_number(ARGO_NUMBER *n, FILE *f) {
    //Get the valid int and valid float fields to use 
    int valid_int = n->valid_int;
    int valid_float = n->valid_float;

    if (valid_int != 0) fprintf(f, "%ld", n->int_value);
    //If valid_float, then normalize the float value of 
    //this argo_num and print it out to the output stream
    else if (valid_float != 0) write_float(n->float_value, f);
    //Print error to stderr if neither
    else return -1;

    return 0;
}
void pretty_newline_detector(FILE *f){
     //If this value is on the top level (ie, level=0) & pretty print is enabled, then print out a 
    //single newline after the value along w/the required indentation
    if (global_options >= 0x30000000){
        //Check if  pretty print is enabled (with, or without a indent arg)
        int indent = (global_options - 0x30000000)*level;
        if (indent>0) fprintf(f, "\n%*c", indent, ' ');
        else fprintf(f, "\n");
    }
}
void write_float(double value, FILE *f){
    int exp = 0;
    int loop_count;
    bool isNegative = (value < 0 ? true : false); //Keeps track of the sign of the number
    value = (isNegative ? value*-1 : value); //Perform abs() on the value, if negative
    //If float>1, then keep diving by 10 until its less than 1
    if (value >= 1){
        while (value >= 1){
            value/=10;
            exp++;
        }
    }
    //If less than 0.1, then do the opposite (ie, mult until >0.1)
    else if (value > 0.0){
        while(value<0.1){
            value*=10;
            exp--;
        }
    }
    //Since the float has been normalized, it will always start with "0.", so that can be printed out first
    isNegative ? fprintf(f, "%s", "-0.") : fprintf(f, "%s", "0.");
    //In order to ensure precision, each digit can be printed out using a loop that constantly 
    //moves the next digit to be printed to be in front of the decimal point
    for (loop_count=16; loop_count > 0; loop_count--){
        //First, get the digit to be printed by multiplying 10 to move the digit up, and cast to an int
        //This is so that the proper digit can be printed, and then subtracted out from the value to repeat the
        //process on the next (now new tenths place) digit
        int digit = (int)(value*=10);
        fprintf(f, "%d", digit);
        value-=digit;
    }
    if(exp != 0) fprintf(f, "%c%d",'e', exp);
}