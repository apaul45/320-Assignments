#ifndef BASIC_H
#define BASIC_H

#include "pbx.h"
#include <semaphore.h>

struct tu{
    int fd; //Socket to communicate with server with
    int ext_no; //Index in pbx array
    int ref_ct;
    TU_STATE state;
    sem_t tu_mutex; //Associated mutex to ensure synchronization
    struct tu *peer; 
};

struct pbx{
    TU *tus[PBX_MAX_EXTENSIONS];
};

int tu_counter;

//PBX mutexes
sem_t pbx_mutex;
//Use the following to coordinate pbx_shutdown
sem_t tu_count_mutex; //Use to lock/unlock the tu counter
sem_t shutdown_mutex; //Use to wait for all tus to unregister before freeing pbx

//TU functions
void set_state_names();
void send_to_client(TU *tu, char *msg);
void hangup_peer(TU *tu);

#endif