/*
 * PBX: simulates a Private Branch Exchange.
 */
#include <stdlib.h>
#include <semaphore.h>
#include "basic.h"
#include "debug.h"
#include <sys/socket.h>

static int find_first_available();
static int find_in_pbx(int ext);

static int find_first_available(){
    int i; 
    for (i = 0; i<PBX_MAX_EXTENSIONS; i++){
        if (!pbx->tus[i]) return i;
    }
    return -1;
}
static int find_in_pbx(int ext){
    int i; 
    for (i = 0; i<PBX_MAX_EXTENSIONS; i++){
        if (pbx->tus[i] && tu_extension(pbx->tus[i]) == ext) return i;
    }
    return -1;
}


/*
 * Initialize a new PBX.
 *
 * @return the newly initialized PBX, or NULL if initialization fails.
 */
#if 1
PBX *pbx_init() {
    tu_counter = 0;
    sem_init(&pbx_mutex, 0, 1);
    sem_init(&tu_count_mutex, 0, 1);
    sem_init(&shutdown_mutex, 0, 1);
    set_state_names();
    PBX *new_pbx = malloc(sizeof(PBX));
    if (new_pbx){
        int i;
        for (i=0; i<PBX_MAX_EXTENSIONS; i++) new_pbx->tus[i] = NULL;
    }
    return new_pbx;
}
#endif

/*
 * Shut down a pbx, shutting down all network connections, waiting for all server
 * threads to terminate, and freeing all associated resources.
 * If there are any registered extensions, the associated network connections are
 * shut down, which will cause the server threads to terminate.
 * Once all the server threads have terminated, any remaining resources associated
 * with the PBX are freed.  The PBX object itself is freed, and should not be used again.
 *
 * @param pbx  The PBX to be shut down.
 */
#if 1
void pbx_shutdown(PBX *pbx) {
    sem_wait(&pbx_mutex);
    debug("pbx shutdown reached \n");
    int i;
    for (i=0; i<PBX_MAX_EXTENSIONS; i++){
        if (pbx->tus[i]) {
            debug("Shutting down tu!\n");
            shutdown(tu_extension(pbx->tus[i]), SHUT_RDWR);
        }
    }
    sem_post(&pbx_mutex);
    sem_wait(&shutdown_mutex); 
    debug("Freeing pbx...");
    free(pbx);
}
#endif

/*
 * Register a telephone unit with a PBX at a specified extension number.
 * This amounts to "plugging a telephone unit into the PBX".
 * The TU is initialized to the TU_ON_HOOK state.
 * The reference count of the TU is increased and the PBX retains this reference
 *for as long as the TU remains registered.
 * A notification of the assigned extension number is sent to the underlying network
 * client.
 *
 * @param pbx  The PBX registry.
 * @param tu  The TU to be registered.
 * @param ext  The extension number on which the TU is to be registered.
 * @return 0 if registration succeeds, otherwise -1.
 */
#if 1
int pbx_register(PBX *pbx, TU *tu, int ext) {
    sem_wait(&pbx_mutex); //Lock mutex
    debug("Reached pbx register\n");
    int index = find_first_available();
    pbx->tus[index] = tu;
    sem_post(&pbx_mutex); //Unlock mutex

    //Make sure to send a notification to the tu using set_extension and ref
    tu_set_extension(tu, 0); 
    tu_ref(tu, "Registered into pbx\n");

    //Increment the tu counter 
    //Once the first tu has been registered, lock the shutdown mutex by incrementing it to 0
    sem_wait(&tu_count_mutex);
    tu_counter++;
    if (tu_counter == 1) sem_wait(&shutdown_mutex); 
    debug("Tu counter is now %d\n", tu_counter);
    sem_post(&tu_count_mutex);
    
    return 0;
}
#endif

/*
 * Unregister a TU from a PBX.
 * This amounts to "unplugging a telephone unit from the PBX".
 * The TU is disassociated from its extension number.
 * Then a hangup operation is performed on the TU to cancel any
 * call that might be in progress.
 * Finally, the reference held by the PBX to the TU is released.
 *
 * @param pbx  The PBX.
 * @param tu  The TU to be unregistered.
 * @return 0 if unregistration succeeds, otherwise -1.
 */
#if 1
int pbx_unregister(PBX *pbx, TU *tu) {
    debug("Pbx unregister reached\n");
    sem_wait(&pbx_mutex); //Lock mutex
    int index = find_in_pbx(tu_extension(tu));
    tu_hangup(tu); //Hangup any calls in progress
    tu_unref(tu, "Unregistering this tu!\n");
    pbx->tus[index] = NULL;
    sem_post(&pbx_mutex); //Unlock mutex

    //Decrement the tu counter 
    //Once all tu's have been unregistered, unlock the shutdown mutex to allow for the pbx to be freed
    sem_wait(&tu_count_mutex);
    tu_counter--;
    if (tu_counter == 0) sem_post(&shutdown_mutex); 
    debug("Tu counter is now %d\n", tu_counter);
    sem_post(&tu_count_mutex);

    return 0;
}
#endif

/*
 * Use the PBX to initiate a call from a specified TU to a specified extension.
 *
 * @param pbx  The PBX registry.
 * @param tu  The TU that is initiating the call.
 * @param ext  The extension number to be called.
 * @return 0 if dialing succeeds, otherwise -1.
 */
#if 1
int pbx_dial(PBX *pbx, TU *tu, int ext) {
    TU *target;
    sem_wait(&pbx_mutex);
    debug("Reached pbx dial\n");
    int index = find_in_pbx(ext);
    target = (index == -1 ? NULL : pbx->tus[index]);
    if (tu_dial(tu, target) < 0){
        sem_post(&pbx_mutex);
        return -1;
    }
    debug("Successfully dialed!\n");
    sem_post(&pbx_mutex);
    return 0;
}
#endif
