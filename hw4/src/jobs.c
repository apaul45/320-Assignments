#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h> //for dup2
#include <wait.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include "stores.h"
#include "debug.h"

static int find_first_index();
static long get_size(ARG* arg);
static int get_command_size(COMMAND* c);
static void create_arg_array(ARG* arg, char* argv[],long size);
static void sigchld_handler(int sig);
static void set_sigmask(int sig);
static void sigio_handler(int sig);
static void determine_job_status(int jobid, int status);

// ** HELPER FUNCTIONS ** 
static int find_first_index(){
    int i;
    set_sigmask(SIGCHLD);
    for (i = 0; i<MAX_JOBS; i++){
        if (!jobs_table[i].pline) break;
    }
    set_sigmask(-1);
    return i;
}
static long get_size(ARG* arg){
    long size = 0;
    ARG* ptr = arg;
    while (ptr){
        size++;
        ptr = ptr->next;
    }
    return size+1;
}
static int get_command_size(COMMAND* c){
    int size = 0;
    COMMAND *dummy = c; 
    for(size = 0; dummy; size++) dummy = dummy->next;
    return size;
}
static void create_arg_array(ARG* arg, char* argv[], long size){
    ARG* ptr = arg;
    int i; 
    for (i = 0; i<size-1; i++){
        argv[i] = eval_to_string(ptr->expr);                   
        ptr = ptr->next;
    }
    argv[i] = '\0'; //Make sure to add null terminator at the end
}
static void determine_exit_status(int* status){
    if (WIFEXITED(*status)) *status = WEXITSTATUS(*status);
    else if (WIFSIGNALED(*status)) *status = WTERMSIG(*status);
    else if (WIFSTOPPED(*status)) *status = WSTOPSIG(*status);
}

static void determine_job_status(int jobid, int status){
    set_sigmask(SIGCHLD);
    if (WIFSIGNALED(status)){
        if (WTERMSIG(status) == SIGKILL) jobs_table[jobid].status = CANCELED;
        if (WTERMSIG(status) == SIGABRT) jobs_table[jobid].status = ABORTED;
    }
    else if (WIFEXITED(status)) jobs_table[jobid].status = COMPLETED;
    set_sigmask(-1);
}

/** HANDLER FUNCTIONS */
static void sigchld_handler(int sig){
    debug("Reached sigchld handler\n");
    pid_t pid; int status;
    //Use WNOHANG to be able to only reap processes that are ALREADY dead
    //Reap as many dead children at once to account for blocked sigchld signals
    while ((pid = waitpid(-1, &status, WNOHANG))>0){
        //Find the job in the jobs table, and then update its status
        int i; 
        for (i=0; i< MAX_JOBS; i++){
            if (jobs_table[i].pline && jobs_table[i].pgid == pid) break;
        }
        //Update the status of this job accordingly
        if (WIFSIGNALED(status)){
            jobs_table[i].status = (WTERMSIG(status) == SIGKILL ? CANCELED : ABORTED);
        }   
        else if (WIFEXITED(status)) jobs_table[i].status = COMPLETED;
    }
}
static void sigio_handler(int sig){
}

static void set_sigmask(int sig){
    sigset_t mask;
    sigemptyset(&mask);
    if (sig > -1) sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_SETMASK, &mask, NULL);
}


/*
 * This is the "jobs" module for Mush.
 * It maintains a table of jobs in various stages of execution, and it
 * provides functions for manipulating jobs.
 * Each job contains a pipeline, which is used to initialize the processes,
 * pipelines, and redirections that make up the job.
 * Each job has a job ID, which is an integer value that is used to identify
 * that job when calling the various job manipulation functions.
 *
 * At any given time, a job will have one of the following status values:
 * "new", "running", "completed", "aborted", "canceled".
 * A newly created job starts out in with status "new".
 * It changes to status "running" when the processes that make up the pipeline
 * for that job have been created.
 * A running job becomes "completed" at such time as all the processes in its
 * pipeline have terminated successfully.
 * A running job becomes "aborted" if the last process in its pipeline terminates
 * with a signal that is not the result of the pipeline having been canceled.
 * A running job becomes "canceled" if the jobs_cancel() function was called
 * to cancel it and in addition the last process in the pipeline subsequently
 * terminated with signal SIGKILL.
 *
 * In general, there will be other state information stored for each job,
 * as required by the implementation of the various functions in this module.
 */

/**
 * @brief  Initialize the jobs module.
 * @details  This function is used to initialize the jobs module.
 * It must be called exactly once, before any other functions of this
 * module are called.
 *
 * @return 0 if initialization is successful, otherwise -1.
 */
int jobs_init(void) {
    //Initialize the jobs table (array) with NULL values
    //This is to help with finding the first open space to insert a job into
    int i; 
    for (i = 0; i< MAX_JOBS; i++){
        Job new_job;
        new_job.pline = NULL;
        jobs_table[i] = new_job;
    }
    return 0;
}

/**
 * @brief  Finalize the jobs module.
 * @details  This function is used to finalize the jobs module.
 * It must be called exactly once when job processing is to be terminated,
 * before the program exits.  It should cancel all jobs that have not
 * yet terminated, wait for jobs that have been cancelled to terminate,
 * and then expunge all jobs before returning.
 *
 * @return 0 if finalization is completely successful, otherwise -1.
 */
int jobs_fini(void) {
    int i; 
    for (i=0; i<MAX_JOBS; i++){
        if (!jobs_table[i].pline) continue;

        int success; 
        if (jobs_table[i].status == CANCELED) success = jobs_wait(i);
        else if (jobs_table[i].status <= RUNNING) success = jobs_cancel(i);

        if (success<0) return -1;
    }
    for (i=0; i<MAX_JOBS; i++){
        if (!jobs_table[i].pline) continue;
        if (jobs_expunge(i)<0) return -1;
    }

    //Free the contents of the program and data stores
    clear_program_store(); clear_data_store();
    return 0;
}

/**
 * @brief  Print the current jobs table.
 * @details  This function is used to print the current contents of the jobs
 * table to a specified output stream.  The output should consist of one line
 * per existing job.  Each line should have the following format:
 *
 *    <jobid>\t<pgid>\t<status>\t<pipeline>
 *
 * where <jobid> is the numeric job ID of the job, <status> is one of the
 * following strings: "new", "running", "completed", "aborted", or "canceled",
 * and <pipeline> is the job's pipeline, as printed by function show_pipeline()
 * in the syntax module.  The \t stand for TAB characters.
 *
 * @param file  The output stream to which the job table is to be printed.
 * @return 0  If the jobs table was successfully printed, -1 otherwise.
 */
int jobs_show(FILE *file) {
    debug("reached jobs show");
    fprintf(file, "%s\t%s\t%s\t%s\n", "JOBID", "PGID", "STATUS", "PIPELINE");

    set_sigmask(SIGCHLD); //Block SIGCHLD before accessing jobs table
    int i; 
    for (i = 0; i<MAX_JOBS; i++){
        if (!jobs_table[i].pline) continue;
        fprintf(file, "%d\t%d\t", i, jobs_table[i].pgid);
        switch(jobs_table[i].status){
            case NEW:
                fprintf(file, "%s\t", "new");
                break;
            case RUNNING: 
                fprintf(file, "%s\t", "running");
                break;
            case COMPLETED: 
                fprintf(file, "%s\t", "completed");
                break;
            case ABORTED: 
                fprintf(file, "%s\t", "aborted");
                break;
            case CANCELED:
                fprintf(file, "%s\t", "canceled");
                break;
        }
        show_pipeline(file, jobs_table[i].pline);
        fprintf(file, "\n");
    }
    set_sigmask(-1); //Unblock SIGCHLD once done
    return 0;
}

/**
 * @brief  Create a new job to run a pipeline.
 * @details  This function creates a new job and starts it running a specified
 * pipeline.  The pipeline will consist of a "leader" process, which is the direct
 * child of the process that calls this function, plus one child of the leader
 * process to run each command in the pipeline.  All processes in the pipeline
 * should have a process group ID that is equal to the process ID of the leader.
 * The leader process should wait for all of its children to terminate before
 * terminating itself.  The leader should return the exit status of the process
 * running the last command in the pipeline as its own exit status, if that
 * process terminated normally.  If the last process terminated with a signal,
 * then the leader should terminate via SIGABRT.
 *
 * If the "capture_output" flag is set for the pipeline, then the standard output
 * of the last process in the pipeline should be redirected to be the same as
 * the standard output of the pipeline leader, and this output should go via a
 * pipe to the main Mush process, where it should be read and saved in the data
 * store as the value of a variable, as described in the assignment handout.
 * If "capture_output" is not set for the pipeline, but "output_file" is non-NULL,
 * then the standard output of the last process in the pipeline should be redirected
 * to the specified output file.   If "input_file" is set for the pipeline, then
 * the standard input of the process running the first command in the pipeline should
 * be redirected from the specified input file.
 *
 * @param pline  The pipeline to be run.  The jobs module expects this object
 * to be valid for as long as it requires, and it expects to be able to free this
 * object when it is finished with it.  This means that the caller should not pass
 * a pipeline object that is shared with any other data structure, but rather should
 * make a copy to be passed to this function.
 * 
 * @return  -1 if the pipeline could not be initialized properly, otherwise the
 * value returned is the job ID assigned to the pipeline.
 */
int jobs_run(PIPELINE *pline) {
    //Setting up job in jobs table
    int id = find_first_index(); //Use index in jobs table as job id
    int job_fd[2];

    //Setting up capture output
    if (pipe(job_fd) < 0) return -1; //Pipe between main and last child process, for capturing output
    fcntl(job_fd[0], F_SETFL, O_NONBLOCK); //Allows main process to not block while waiting 
    fcntl(job_fd[0], F_SETFL, O_ASYNC); //Will invoke SIGIO handler when job_fd[0] ready
    fcntl(job_fd[0], __F_SETOWN, getpid()); //Assigns pipe ownership to main process
    
    //Setting up signal handlers
    signal(SIGCHLD, sigchld_handler);
    signal(SIGIO, sigio_handler);

    int pid = fork(); //Create the "leader process"

    if (pid < 0) return -1;

    //Add the new job to the jobs_table upon creation in the main process
    else if (pid != 0){
        Job new_job;
        new_job.pline = copy_pipeline(pline);
        new_job.pgid = pid; //Process group id should be the leader's pid
        new_job.status = RUNNING;
        new_job.job_id = id;
        new_job.attempted_cancel = false;
        new_job.pipe[0] = job_fd[0]; new_job.pipe[1] = job_fd[1];
        set_sigmask(SIGCHLD); //Make sure to block SIGCHLD before writing to jobs table
        jobs_table[id] = new_job;
        set_sigmask(-1); //Unblock SIGCHLD
    }

    //Create child processes from the leader process & run each command using execvp
    else{
        //Make sure to set the process group of the leader process 
        //By default, its children processes will be in the same process group
        setpgid(0, 0);

        signal(SIGCHLD, SIG_DFL); //Ignore sigchld signals, since leader reaps children

        //Create n-1 pipes for pipeline input and output redirection
        int size = 2*get_command_size(pline->commands), pipes[size], i;
        for (i=0; i<(size-1); i+=2){
            int fd[2]; pipe(fd);
            pipes[i] = fd[0]; pipes[i+1] = fd[1];
        }

        COMMAND* head = pline->commands;
        int pid, counter = 0;
        while (head){
            pid = fork();
            if (pid < 0) _exit(-1);
            //Run this command using execvp for this new child process
            if (pid == 0){
                int read = (2*counter)-2, write = (2*counter) + 1;

                //Close unused pipes
                int j;
                for (j=0; j<size; j++){
                    if (j != read && j != write) close(pipes[j]);
                }

                //Set up input redirection for first child if input file specified
                if (head == pline->commands && pline->input_file){
                    int input_fd = open(pline->input_file, O_RDWR|O_CREAT, 0777);
                    if (input_fd == -1) abort();
                    dup2(input_fd, STDIN_FILENO);
                }
                //Set up input redirection for pipelines
                else if (head != pline->commands) dup2(pipes[read], STDIN_FILENO);

                //Set up output redirection for last child if capture output isn't specified but output file not NULL
                if (!head->next && !pline->capture_output && pline->output_file){
                    debug("Reached output redirection case\n");
                    close(pipes[write]); //Close write end of pipe since unused
                    int output_fd = open(pline->output_file, O_RDWR | O_CREAT, 0777);
                    if (output_fd == -1) abort();
                    dup2(output_fd, STDOUT_FILENO);
                }   
                //Set up output redirection for pipelines, close main pipe
                else if (head->next){
                    dup2(pipes[write], STDOUT_FILENO);
                    close(job_fd[0]); close(job_fd[1]);
                }
                //Set up output capture
                else if (pline->capture_output){
                    debug("Reached capture output case\n");
                    close(job_fd[0]);
                    dup2(job_fd[1], STDOUT_FILENO);
                }

                //First arg in the arg list of this command is the name of the command itself
                //Should be first argument of execvp
                long size = get_size(head->args);
                char* argv[size];
                create_arg_array(head->args, argv, size);
                if (execvp((const char *)head->args->expr->members.value, argv)<0){
                    perror("execvp"); abort();
                }
            }
            counter++; 
            head = head->next;
        }
        for (i=0; i<size; i++) close(pipes[i]); //Close all of the pipes for leader
        //Wait for all of the child processes to finish before terminating the leader process
        pid_t exit_status; int child_status, actual_child_status = 0;
        while ((exit_status = wait(&child_status)) > 0){
            if (WIFEXITED(child_status)) debug("Child %d terminated with %d\n", exit_status, child_status);
            actual_child_status = (exit_status == pid ? child_status : actual_child_status);
        }
        if (WIFSIGNALED(actual_child_status)) abort();
        determine_exit_status(&actual_child_status);
        _exit(actual_child_status); //Exit with the status of the last ran child process
    }
    return id;
}

/**
 * @brief  Wait for a job to terminate.
 * @details  This function is used to wait for the job with a specified job ID
 * to terminate.  A job has terminated when it has entered the COMPLETED, ABORTED,
 * or CANCELED state.
 *
 * @param  jobid  The job ID of the job to wait for.
 * @return  the exit status of the job leader, as returned by waitpid(),
 * or -1 if any error occurs that makes it impossible to wait for the specified job.
 */
int jobs_wait(int jobid) {
    set_sigmask(SIGCHLD);
    int status, group_id = jobs_table[jobid].pgid;
    set_sigmask(-1);

    int exit_status = waitpid(group_id, &status, 0);

    //Check if a handler reaped the child before the waitpid above could
    if (exit_status < 0){
        set_sigmask(SIGCHLD);
        int jobs_status = jobs_table[jobid].status;
        set_sigmask(-1); 

        if (jobs_status > RUNNING) status = (jobs_status == ABORTED ? SIGABRT : jobs_status == COMPLETED ? 0 : SIGKILL);
        else return -1;
    }
    //Update the job's status depending on how it exited
    if (exit_status>0) determine_job_status(jobid, status);
    determine_exit_status(&status);
    return (status < 0 ? -1 : status);
}

/**
 * @brief  Poll to find out if a job has terminated.
 * @details  This function is used to poll whether the job with the specified ID
 * has terminated.  This is similar to jobs_wait(), except that this function returns
 * immediately without waiting if the job has not yet terminated.
 *
 * @param  jobid  The job ID of the job to wait for.
 * @return  the exit status of the job leader, as returned by waitpid(), if the job
 * has terminated, or -1 if the job has not yet terminated or if any other error occurs.
 */
int jobs_poll(int jobid) {
    //jobs_poll is like jobs_wait, but the options argument of waitpid is specified
    //to include the WNOHANG option, which returns immediately if no child exited 
    int status, pgid;
    set_sigmask(SIGCHLD); 
    pgid = jobs_table[jobid].pgid;
    set_sigmask(-1);

    //WNOHANG is automatically OR'ed with zero (default option that waits until termination)
    //Check if signal handler reaped the job leader before waitpid could
    pid_t pid = waitpid(pgid, &status, WNOHANG);
    if (pid <= 0){
        set_sigmask(SIGCHLD);
        int jobs_status = jobs_table[jobid].status;
        set_sigmask(-1); 

        if (jobs_status > RUNNING) status = (jobs_status == ABORTED ? SIGABRT : jobs_status == COMPLETED ? 0 : SIGKILL);
        else return -1;
    }
    if (pid > 0) determine_job_status(jobid, status);
    determine_exit_status(&status);
    return status;
}

/**
 * @brief  Expunge a terminated job from the jobs table.
 * @details  This function is used to expunge (remove) a job that has terminated from
 * the jobs table, so that space in the table can be used to start some new job.
 * In order to be expunged, a job must have terminated; if an attempt is made to expunge
 * a job that has not yet terminated, it is an error.  Any resources (exit status,
 * open pipes, captured output, etc.) that were being used by the job are finalized
 * and/or freed and will no longer be available.
 *
 * @param  jobid  The job ID of the job to expunge.
 * @return  0 if the job was successfully expunged, -1 if the job could not be expunged.
 */
int jobs_expunge(int jobid) {
    debug("Reached jobs expunge\n");
    set_sigmask(SIGCHLD);
    int return_val = -1;
    if (jobs_table[jobid].status != NEW && jobs_table[jobid].status != RUNNING){
        free(jobs_table[jobid].pline);
        jobs_table[jobid].pline = NULL;
        return_val = 0;
    }
    set_sigmask(-1);
    debug("Jobs expunge exiting with status %d\n", return_val);
    return return_val;
}

/**
 * @brief  Attempt to cancel a job.
 * @details  This function is used to attempt to cancel a running job.
 * In order to be canceled, the job must not yet have terminated and there
 * must not have been any previous attempt to cancel the job.
 * Cancellation is attempted by sending SIGKILL to the process group associated
 * with the job.  Cancellation becomes successful, and the job actually enters the canceled
 * state, at such subsequent time as the job leader terminates as a result of SIGKILL.
 * If after attempting cancellation, the job leader terminates other than as a result
 * of SIGKILL, then cancellation is not successful and the state of the job is either
 * COMPLETED or ABORTED, depending on how the job leader terminated.
 *
 * @param  jobid  The job ID of the job to cancel.
 * @return  0 if cancellation was successfully initiated, -1 if the job was already
 * terminated, a previous attempt had been made to cancel the job, or any other
 * error occurred.
 */
int jobs_cancel(int jobid) {
    //Block the sigchld handler from being invoked when reading jobs table
    set_sigmask(SIGCHLD);
    int return_val = -1;
    if (jobs_table[jobid].status <= RUNNING && !jobs_table[jobid].attempted_cancel){
        debug("Reached running job in jobs cancel\n");
        return_val = kill(0-jobs_table[jobid].pgid, SIGKILL);
        if (return_val == 0) jobs_table[jobid].status = CANCELED; //If successful, set to canceled
        else jobs_table[jobid].attempted_cancel = true;
        debug("Value from kill is %d\n", return_val);
    }
    set_sigmask(-1); //Unmask so that sigchld handler could now reap this leader
    return return_val;
}

/**
 * @brief  Get the captured output of a job.
 * @details  This function is used to retrieve output that was captured from a job
 * that has terminated, but that has not yet been expunged.  Output is captured for a job
 * when the "capture_output" flag is set for its pipeline.
 *
 * @param  jobid  The job ID of the job for which captured output is to be retrieved.
 * @return  The captured output, if the job has terminated and there is captured
 * output available, otherwise NULL.
 */
char *jobs_get_output(int jobid) {
    debug("Beginning of capture output\n");

    set_sigmask(SIGCHLD);
    close(jobs_table[jobid].pipe[1]);
    char *stream_ptr, *output = NULL, input[1]; size_t length;
    FILE* stream = open_memstream(&stream_ptr, &length);

    while (read(jobs_table[jobid].pipe[0], input, 1) > 0 && errno != EWOULDBLOCK) fprintf(stream, "%c", input[0]);
    fclose(stream); 
    if (length == 0){
        free(stream_ptr);
        set_sigmask(-1);
        return NULL;
    }
    output = malloc(length+1);
    memcpy(output, stream_ptr, length);
    debug("Content from output capture: %s\n", output);
    free(stream_ptr);
    set_sigmask(-1);
    return output;
}

/**
 * @brief  Pause waiting for a signal indicating a potential job status change.
 * @details  When this function is called it blocks until some signal has been
 * received, at which point the function returns.  It is used to wait for a
 * potential job status change without consuming excessive amounts of CPU time.
 *
 * @return -1 if any error occurred, 0 otherwise.
 */
int jobs_pause(void) {
    //Block all signals except for sigchld and sigio 
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGCHLD);
    sigdelset(&mask, SIGIO);
    sigsuspend(&mask);
    return (errno == EINTR ? -1 : 0);
}