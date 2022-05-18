/**
 * This header file will contain the definitions
 * and data structures of each store, which include the 
 * programs and data stores, and "job" table for processes
 * 
 */
#include "mush.h"
#include <stdbool.h>
#ifndef STORES_H
#define STORES_H
/**
 * The program store will be maintained as sorted, 
 * singly linked list made up of nodes that contain
 * a STMT statement as defined in syntax.h, and a 
 * pointer to next
 */
typedef struct Statement{
    STMT* statement;
    struct Statement* next;
}Statement;

Statement* stm_head;

#define MAX_DATA 100

//Data store will be maintained as an array of Variables
typedef struct{
    char* name;
    char* value; //Value is always represented as a string
}Variable;


typedef enum{
    NEW, RUNNING, COMPLETED, ABORTED, CANCELED
}JOB_STATUS;

typedef struct{
    JOB_STATUS status;
    int job_id;
    pid_t pgid; //process group id, set by leader process
    PIPELINE* pline; 
    bool attempted_cancel;
    int pipe[2]; //Hold file descriptor for capturing output
}Job;


//As jobs only consist of a integer ID, an array of length MAX_JOBS will 
//be used to represent the jobs table
Job jobs_table[MAX_JOBS];

int clear_program_store();
int clear_data_store();

#endif