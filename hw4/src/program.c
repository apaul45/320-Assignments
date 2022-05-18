#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "stores.h"
#include "debug.h"

static Statement* find_in_store(int lineno);

//Initialize the head and program counter
Statement* stm_head = NULL;
int program_counter = -1;

int clear_program_store(){
    Statement* ptr = stm_head;
    while(ptr){
        Statement *tmp = ptr->next;
        free(ptr->statement);
        free(ptr);
        ptr = tmp;
    }
    return 0;
}

/*
 * This is the "program store" module for Mush.
 * It maintains a set of numbered statements, along with a "program counter"
 * that indicates the current point of execution, which is either before all
 * statements, after all statements, or in between two statements.
 * There should be no fixed limit on the number of statements that the program
 * store can hold.
 */

/**
 * @brief  Output a listing of the current contents of the program store.
 * @details  This function outputs a listing of the current contents of the
 * program store.  Statements are listed in increasing order of their line
 * number.  The current position of the program counter is indicated by
 * a line containing only the string "-->" at the current program counter
 * position.
 *
 * @param out  The stream to which to output the listing.
 * @return  0 if successful, -1 if any error occurred.
 */
int prog_list(FILE *out) {
    Statement* node = stm_head;
    bool found = false; //Use to identify the statement that the program counter is pointing to
    while (node){
        if (!found && node->statement->lineno > program_counter){
            fprintf(out, "%s\n", "-->");
            found = true;
        } 
        show_stmt(out, node->statement);
        node = node->next;
    }

    if (!found) fprintf(out, "%s\n", "-->"); //If pc was out of bounds, print arrow here
    return 0;
}

/**
 * @brief  Insert a new statement into the program store.
 * @details  This function inserts a new statement into the program store.
 * The statement must have a line number.  If the line number is the same as
 * that of an existing statement, that statement is replaced.
 * The program store assumes the responsibility for ultimately freeing any
 * statement that is inserted using this function.
 * Insertion of new statements preserves the value of the program counter:
 * if the position of the program counter was just before a particular statement
 * before insertion of a new statement, it will still be before that statement
 * after insertion, and if the position of the program counter was after all
 * statements before insertion of a new statement, then it will still be after
 * all statements after insertion.
 *
 * @param stmt  The statement to be inserted.
 * @return  0 if successful, -1 if any error occurred.
 */
int prog_insert(STMT *stmt) {
    // TO BE IMPLEMENTED
    Statement *new_stm = malloc(sizeof(Statement));
    if (!new_stm) return -1; //Return -1 if error occurred when allocating new statement
    new_stm->statement = stmt;

    if (!stm_head) stm_head = new_stm;
    else if (new_stm->statement->lineno < stm_head->statement->lineno){
        new_stm->next = stm_head;
        stm_head = new_stm;
    }
    else{
        Statement new_val;
        Statement* val = &new_val;
        val->next = stm_head;
        //Search for the correct position of this new statement in the sorted program store
        while(val->next && val->next->statement->lineno <= new_stm->statement->lineno) val = val->next;
        //If existing node, replace old statement w/new one 
        if (val->statement->lineno == new_stm->statement->lineno) {
            free(val->statement);
            val->statement = new_stm->statement;
            free(new_stm);
        }
        else{
            new_stm->next = val->next;
            val->next = new_stm;
        }
    }
    return 0;
}

/**
 * @brief  Delete statements from the program store.
 * @details  This function deletes from the program store statements whose
 * line numbers fall in a specified range.  Any deleted statements are freed.
 * Deletion of statements preserves the value of the program counter:
 * if before deletion the program counter pointed to a position just before
 * a statement that was not among those to be deleted, then after deletion the
 * program counter will still point the position just before that same statement.
 * If before deletion the program counter pointed to a position just before
 * a statement that was among those to be deleted, then after deletion the
 * program counter will point to the first statement beyond those deleted,
 * if such a statement exists, otherwise the program counter will point to
 * the end of the program.
 *
 * @param min  Lower end of the range of line numbers to be deleted.
 * @param max  Upper end of the range of line numbers to be deleted.
 */
int prog_delete(int min, int max) {
    if (max < min) return -1;

    //Need to use sentinel so that the node right before the first in the range can be relinked to the one following the range
    Statement head, tail, *dummy_head = &head, *dummy_tail = &tail; 
    dummy_head->next = stm_head;
    while (dummy_head->next && dummy_head->next->statement->lineno < min) dummy_head = dummy_head->next;
    dummy_tail = dummy_head;
    while (dummy_tail->next && dummy_tail->next->statement->lineno <= max) dummy_tail = dummy_tail->next;
    
    if (dummy_head && dummy_tail){
        Statement *tmp_head = dummy_head, *tmp_head2 = dummy_head;

        tmp_head = tmp_head->next;
        while (tmp_head != dummy_tail){
            tmp_head2 = tmp_head->next;
            free_stmt(tmp_head->statement);
            free(tmp_head);
            tmp_head = tmp_head2;
        }

        if (dummy_head == &head) stm_head = dummy_tail->next;
        else dummy_head->next = dummy_tail->next;
        free(dummy_tail);
    }
    return 0;
}

/**
 * @brief  Reset the program counter to the beginning of the program.
 * @details  This function resets the program counter to point just
 * before the first statement in the program.
 */
void prog_reset(void) {
    program_counter = (stm_head ? stm_head->statement->lineno - 1 : -1);
}

/**
 * @brief  Fetch the next program statement.
 * @details  This function fetches and returns the first program
 * statement after the current program counter position.  The program
 * counter position is not modified.  The returned pointer should not
 * be used after any subsequent call to prog_delete that deletes the
 * statement from the program store.
 *
 * @return  The first program statement after the current program
 * counter position, if any, otherwise NULL.
 */
STMT *prog_fetch(void) {
    Statement* stmt = find_in_store(program_counter);
    return (stmt ? stmt->statement : NULL);
}

/**
 * @brief  Advance the program counter to the next existing statement.
 * @details  This function advances the program counter by one statement
 * from its original position and returns the statement just after the
 * new position.  The returned pointer should not be used after any
 * subsequent call to prog_delete that deletes the statement from the
 * program store.
 *
 * @return The first program statement after the new program counter
 * position, if any, otherwise NULL.
 */
STMT *prog_next() {
    Statement *old_pc = find_in_store(program_counter);
    program_counter = (old_pc ? old_pc->statement->lineno : program_counter);
    return (old_pc ? old_pc->next ? old_pc->next->statement : NULL : NULL);
}

/**
 * @brief  Perform a "go to" operation on the program store.
 * @details  This function performs a "go to" operation on the program
 * store, by resetting the program counter to point to the position just
 * before the statement with the specified line number.
 * The statement pointed at by the new program counter is returned.
 * If there is no statement with the specified line number, then no
 * change is made to the program counter and NULL is returned.
 * Any returned statement should only be regarded as valid as long
 * as no calls to prog_delete are made that delete that statement from
 * the program store.
 *
 * @return  The statement having the specified line number, if such a
 * statement exists, otherwise NULL.
 */
STMT *prog_goto(int lineno) {
    Statement* goto_stm = find_in_store(lineno);
    program_counter = (goto_stm ? goto_stm->statement->lineno - 1 : program_counter);
    return (goto_stm ? goto_stm->statement : NULL);
}

/**
 * @brief Finds the statement with the specified line number,
 * or the first one following the position of the program counter
 * 
 * @param lineno 
 * @return Statement* The statement with the specified line number, or the 
 * first one following the program counter
 */
static Statement* find_in_store(int lineno){
    Statement *head = stm_head;
    
    //If lineno is the program counter, then search for the first statement following it
    if (lineno == program_counter){
        while (head && head->statement->lineno <= lineno) head = head->next;
    }
    else{
        while (head && head->statement->lineno != lineno) head = head->next;
    }
    return head;
}