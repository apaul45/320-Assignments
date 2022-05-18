/*
 * TU: simulates a "telephone unit", which interfaces a client with the PBX.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "basic.h"
#include "debug.h"
#include <limits.h>
#include <string.h>
#define MAXLINE 8192

void send_to_client(TU *tu, char *msg){
    debug("Reached send to client for tu with ext %d\n", tu->fd);
    if (!msg){
        char buf[MAXLINE];
        if (tu->state == TU_ON_HOOK || tu->state == TU_CONNECTED)
            sprintf(buf, "%s %d\n", tu_state_names[tu->state], tu->fd);
        else
            sprintf(buf, "%s\n", tu_state_names[tu->state]);
        write(tu->fd, buf, strlen(buf));
    }
    else{
        debug("Sending message containing: %s\n", msg);
        write(tu->fd, msg, strlen(msg));
        free(msg);
    }
    debug("Exiting send to client for tu with ext %d\n", tu->fd);
}
void set_state_names(){
    tu_state_names[TU_ON_HOOK] = "ON HOOK";
    tu_state_names[TU_RINGING] = "RINGING";
    tu_state_names[TU_DIAL_TONE] = "DIAL TONE";
    tu_state_names[TU_RING_BACK] = "RING BACK";
    tu_state_names[TU_BUSY_SIGNAL] = "BUSY SIGNAL";
    tu_state_names[TU_CONNECTED] = "CONNECTED";
    tu_state_names[TU_ERROR] = "ERROR";
}


/*
 * Initialize a TU
 *
 * @param fd  The file descriptor of the underlying network connection.
 * @return  The TU, newly initialized and in the TU_ON_HOOK state, if initialization
 * was successful, otherwise NULL.
 */
#if 1
TU *tu_init(int fd) {
    debug("Initializing a new TU with file descriptor %d\n", fd);
    TU *new_tu = malloc(sizeof(TU));
    if (new_tu){
        new_tu->fd = fd;
        new_tu->state = TU_ON_HOOK;
        new_tu->ref_ct = 0;
        new_tu->peer = NULL;
        new_tu->ext_no = -1;
        sem_init(&new_tu->tu_mutex, 0, 1);
        debug("Successfully initialized a new tu\n");
    }
    return new_tu;
}
#endif

/*
 * Increment the reference count on a TU.
 *
 * @param tu  The TU whose reference count is to be incremented
 * @param reason  A string describing the reason why the count is being incremented
 * (for debugging purposes).
 */
#if 1
void tu_ref(TU *tu, char *reason) {
    sem_wait(&tu->tu_mutex); //Lock mutex
    debug("%s with extension %d\n", reason, tu->ext_no);
    tu->ref_ct++;
    debug("Reference count for this tu %d is now %d \n", tu->fd, tu->ref_ct);
    sem_post(&tu->tu_mutex);
}
#endif

/*
 * Decrement the reference count on a TU, freeing it if the count becomes 0.
 *
 * @param tu  The TU whose reference count is to be decremented
 * @param reason  A string describing the reason why the count is being decremented
 * (for debugging purposes).
 */
#if 1
void tu_unref(TU *tu, char *reason) {
    sem_wait(&tu->tu_mutex); //Lock mutex
    debug("%s with extension %d\n", reason, tu->ext_no);
    --tu->ref_ct;
    debug("Reference count for this tu %d is now %d \n", tu->fd, tu->ref_ct);
    //Once 0, free the tu
    if (tu->ref_ct == 0){
        debug("Freeing the tu in unref!\n");
        sem_post(&tu->tu_mutex);
        free(tu);
    }
    else sem_post(&tu->tu_mutex);
}
#endif

/*
 * Get the file descriptor for the network connection underlying a TU.
 * This file descriptor should only be used by a server to read input from
 * the connection.  Output to the connection must only be performed within
 * the PBX functions.
 *
 * @param tu
 * @return the underlying file descriptor, if any, otherwise -1.
 */
#if 1
int tu_fileno(TU *tu) {
    if (!tu) return -1;
    int fileno;
    //sem_wait(&tu->tu_mutex); //Lock this tu's mutex
    fileno = tu->fd;
    //sem_post(&tu->tu_mutex); //Unlock tu's mutex
    return fileno;
}
#endif

/*
 * Get the extension number for a TU.
 * This extension number is assigned by the PBX when a TU is registered
 * and it is used to identify a particular TU in calls to tu_dial().
 * The value returned might be the same as the value returned by tu_fileno(),
 * but is not necessarily so.
 *
 * @param tu
 * @return the extension number, if any, otherwise -1.
 */
#if 1
int tu_extension(TU *tu) {
    if (!tu) return -1;
    int ext;
    sem_wait(&tu->tu_mutex); //Lock this tu's mutex
    ext = tu->ext_no;
    sem_post(&tu->tu_mutex); //Unlock tu's mutex
    return ext;
}
#endif

/*
 * Set the extension number for a TU.
 * A notification is set to the client of the TU.
 * This function should be called at most once one any particular TU.
 *
 * @param tu  The TU whose extension is being set.
 */
#if 1
int tu_set_extension(TU *tu, int ext) {
    if (!tu) return -1;
    sem_wait(&tu->tu_mutex); //Lock mutex
    tu->ext_no = tu->fd;
    send_to_client(tu, NULL);
    sem_post(&tu->tu_mutex);
    debug("Successfully set this tu's extension number\n");
    return 0;
}
#endif

/*
 * Initiate a call from a specified originating TU to a specified target TU.
 *   If the originating TU is not in the TU_DIAL_TONE state, then there is no effect.
 *   If the target TU is the same as the originating TU, then the TU transitions
 *     to the TU_BUSY_SIGNAL state.
 *   If the target TU already has a peer, or the target TU is not in the TU_ON_HOOK
 *     state, then the originating TU transitions to the TU_BUSY_SIGNAL state.
 *   Otherwise, the originating TU and the target TU are recorded as peers of each other
 *     (this causes the reference count of each of them to be incremented),
 *     the target TU transitions to the TU_RINGING state, and the originating TU
 *     transitions to the TU_RING_BACK state.
 *
 * In all cases, a notification of the resulting state of the originating TU is sent to
 * to the associated network client.  If the target TU has changed state, then its client
 * is also notified of its new state.
 *
 * If the caller of this function was unable to determine a target TU to be called,
 * it will pass NULL as the target TU.  In this case, the originating TU will transition
 * to the TU_ERROR state if it was in the TU_DIAL_TONE state, and there will be no
 * effect otherwise.  This situation is handled here, rather than in the caller,
 * because here we have knowledge of the current TU state and we do not want to introduce
 * the possibility of transitions to a TU_ERROR state from arbitrary other states,
 * especially in states where there could be a peer TU that would have to be dealt with.
 *
 * @param tu  The originating TU.
 * @param target  The target TU, or NULL if the caller of this function was unable to
 * identify a TU to be dialed.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
#if 1
int tu_dial(TU *tu, TU *target) {
    if (!target){
        sem_wait(&tu->tu_mutex);
        if (tu->state == TU_DIAL_TONE) tu->state = TU_ERROR;
        send_to_client(tu, NULL);
        sem_post(&tu->tu_mutex);
        return -1;
    }
    else if (tu == target){
        sem_wait(&tu->tu_mutex);
        tu->state = TU_BUSY_SIGNAL;
        send_to_client(tu, NULL);
        sem_post(&tu->tu_mutex);
        return 0;
    }

    //Lock both mutexes
    sem_wait(&tu->tu_mutex);
    sem_wait(&target->tu_mutex);

    debug("Originating TU's state: %d\n", tu->state);
    debug("Target TU's state: %d\n", target->state);


    if (target->ref_ct > 1 || target->state != TU_ON_HOOK) {
        debug("Reached case in tu dial where target has peer/not in on hook\n");
        tu->state = TU_BUSY_SIGNAL;
    }
    else if (tu->state == TU_DIAL_TONE){
        debug("Establishing peers in tu dial!\n");
        tu->peer = target; target->peer = tu;
        tu->ref_ct++; target->ref_ct++;
        tu->state = TU_RING_BACK;
        target->state = TU_RINGING;
        send_to_client(target, NULL);
    }
    send_to_client(tu, NULL);

    //Unlock mutexes in opposite order, to prevent deadlock
    sem_post(&target->tu_mutex);
    sem_post(&tu->tu_mutex);

    return 0;    
}
#endif

/*
 * Take a TU receiver off-hook (i.e. pick up the handset).
 *   If the TU is in neither the TU_ON_HOOK state nor the TU_RINGING state,
 *     then there is no effect.
 *   If the TU is in the TU_ON_HOOK state, it goes to the TU_DIAL_TONE state.
 *   If the TU was in the TU_RINGING state, it goes to the TU_CONNECTED state,
 *     reflecting an answered call.  In this case, the calling TU simultaneously
 *     also transitions to the TU_CONNECTED state.
 *
 * In all cases, a notification of the resulting state of the specified TU is sent to
 * to the associated network client.  If a peer TU has changed state, then its client
 * is also notified of its new state.
 *
 * @param tu  The TU that is to be picked up.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
#if 1
int tu_pickup(TU *tu) {
    sem_wait(&tu->tu_mutex);
    debug("Reached tu pickup\n");
    if (tu->state == TU_ON_HOOK) tu->state = TU_DIAL_TONE;
    else if (tu->state == TU_RINGING) {
        tu->state = TU_CONNECTED;
        sem_wait(&tu->peer->tu_mutex);
        tu->peer->state = TU_CONNECTED;
        send_to_client(tu->peer, NULL);
        sem_post(&tu->peer->tu_mutex);
    }
    send_to_client(tu, NULL);

    sem_post(&tu->tu_mutex);
    return 0;
}
#endif

/*
 * Hang up a TU (i.e. replace the handset on the switchhook).
 *
 *   If the TU is in the TU_CONNECTED or TU_RINGING state, then it goes to the
 *     TU_ON_HOOK state.  In addition, in this case the peer TU (the one to which
 *     the call is currently connected) simultaneously transitions to the TU_DIAL_TONE
 *     state.
 *   If the TU was in the TU_RING_BACK state, then it goes to the TU_ON_HOOK state.
 *     In addition, in this case the calling TU (which is in the TU_RINGING state)
 *     simultaneously transitions to the TU_ON_HOOK state.
 *   If the TU was in the TU_DIAL_TONE, TU_BUSY_SIGNAL, or TU_ERROR state,
 *     then it goes to the TU_ON_HOOK state.
 *
 * In all cases, a notification of the resulting state of the specified TU is sent to
 * to the associated network client.  If a peer TU has changed state, then its client
 * is also notified of its new state.
 *
 * @param tu  The tu that is to be hung up.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
#if 1
int tu_hangup(TU *tu) {
    sem_wait(&tu->tu_mutex);
    debug("Reached tu hangup\n");
    if (tu->state == TU_CONNECTED || tu->state == TU_RINGING || tu->state == TU_RING_BACK)
        hangup_peer(tu);
    tu->state = TU_ON_HOOK;
    send_to_client(tu, NULL);
    sem_post(&tu->tu_mutex);
    return 0;
}
#endif

//Helper for tu_hangup
void hangup_peer(TU *tu){
    sem_wait(&tu->peer->tu_mutex);
    debug("Reached tu hangup helper\n");
    tu->peer->state = (tu->state == TU_RING_BACK ? TU_ON_HOOK : TU_DIAL_TONE);
    //Reset peers once done updating the peer's state
    tu->ref_ct--; tu->peer->ref_ct--;
    send_to_client(tu->peer, NULL);
    sem_post(&tu->peer->tu_mutex);    
}

/*
 * "Chat" over a connection.
 *
 * If the state of the TU is not TU_CONNECTED, then nothing is sent and -1 is returned.
 * Otherwise, the specified message is sent via the network connection to the peer TU.
 * In all cases, the states of the TUs are left unchanged and a notification containing
 * the current state is sent to the TU sending the chat.
 *
 * @param tu  The tu sending the chat.
 * @param msg  The message to be sent.
 * @return 0  If the chat was successfully sent, -1 if there is no call in progress
 * or some other error occurs.
 */
#if 1
int tu_chat(TU *tu, char *msg) {
    debug("Reached the beginning of tu chat\n");
    int return_val = 0;
    sem_wait(&tu->tu_mutex);

    if (tu->state != TU_CONNECTED || !tu->peer) return_val = -1;
    else{
        sem_wait(&tu->peer->tu_mutex);
        debug("Reached successful tu chat! \n");
        char* chat_str = malloc(6+strlen(msg));
        sprintf(chat_str, "%s%s\n", "CHAT ", msg);
        debug("Sending message: %s\n", chat_str);
        send_to_client(tu->peer, chat_str);
        sem_post(&tu->peer->tu_mutex);
    }
    send_to_client(tu, NULL);
    debug("Unlocking mutex\n");
    sem_post(&tu->tu_mutex);
    return return_val;
}
#endif
