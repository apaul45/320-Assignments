/*
 * "PBX" server module.
 * Manages interaction with a client telephone unit (TU).
 */
#include <stdlib.h>
#include <stdio.h>
#include "debug.h"
#include "pbx.h"
#include "server.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

//Parsing macros from hw1
#define is_digit(c) ((c) >= '0' && (c) <= '9')
#define is_whitespace(c) ((c) == ' ' || (c) == '\n' || (c) == '\r' || c == '\t')

/*
 * Thread function for the thread that handles interaction with a client TU.
 * This is called after a network connection has been made via the main server
 * thread and a new thread has been created to handle the connection.
 */
#if 1
void *pbx_client_service(void *arg) {
    int fd = *(int *)arg;
    free(arg);
    TU *new_tu = tu_init(fd);
    if (pbx_register(pbx, new_tu, fd) < 0) return NULL;

    char *ptr, *ptr2, input[1]; 
    size_t len = 0;
    FILE *f = fdopen(fd, "r");

    while(1){
        FILE *stream = open_memstream(&ptr, &len);
        while ((input[0] = fgetc(f)) != EOF && input[0] != '\n') 
            fprintf(stream, "%c", input[0]);
        fclose(stream);

        if (len != 0){
            debug("Value of this client command: %s\n", ptr);
            if (strcmp(ptr, "pickup\r") == 0) tu_pickup(new_tu);
            else if (strcmp(ptr, "hangup\r") == 0) tu_hangup(new_tu);
            else if (len >= 5 && strncmp(ptr, "chat", 4) == 0) {
                debug("Chat command reached with message %s\n", ptr + 5);
                tu_chat(new_tu, ptr+5);
            }
            else if (len >= 6 && (ptr2 = strstr(ptr, "dial")) != NULL){
                int num_length = 0;
                char *tmp = ptr2 + 5;
                while (is_digit(tmp[0]) != 0){
                    debug("Value of char = %c\n", tmp[0]);
                    num_length++;
                    tmp++;
                }
                if (num_length > 0){
                    debug("Valid number included in dial command\n");
                    char* num = malloc(num_length + 1);
                    if (num){
                        memcpy(num, ptr2+5, num_length);
                        pbx_dial(pbx, new_tu, atoi(num));
                    }
                    free(num);
                }
                else {
                    char *tmp = ptr2+5;
                    if (is_whitespace(tmp[0]) == 0) pbx_dial(pbx, new_tu, -1); //Only cause error transition if a nonnumerical was given as an argument
                }
            }
        }
        
        free(ptr);
        //Check if connection has been terminated-- read would've returned EOF if so
        if (input[0] == EOF){
             debug("End of server reached\n");
             break;
        }
    }
    fclose(f);
    pbx_unregister(pbx, new_tu);
    return NULL;
}
#endif