#include <stdlib.h>
#include <unistd.h>
#include "errno.h"
#include "pbx.h"
#include "server.h"
#include "debug.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

static void terminate(int status);
static int open_listenfd(char *port);
void *thread(void* vargp);

static volatile sig_atomic_t terminate_flag = 0;
static int *connfd = NULL;
void sighup_handler(int sig) {terminate_flag = 1;}

/*
 * "PBX" telephone exchange simulation.
 *
 * Usage: pbx <port>
 */
int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    char *port;

    if (argc >= 3 && strcmp(argv[1], "-p") == 0){
        char* temp;
        long s = strtol(argv[2], &temp, 10);
        if (errno != 0 || s<1024)  exit(EXIT_SUCCESS);
        else port = argv[2];
        debug("Port number is %ld\n", s);
    }
    else exit(EXIT_SUCCESS);

    // Perform required initialization of the PBX module.
    debug("Initializing PBX...");
    pbx = pbx_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function pbx_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    //Setting up SIGHUP handler
    struct sigaction act = {0};
    act.sa_handler = &sighup_handler;
    sigaction(SIGHUP, &act, NULL);

    //Setting up SIGPIPE handler
    struct sigaction pipe = {0};
    pipe.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &pipe, NULL);

    int listenfd = open_listenfd(port);
    socklen_t clientlen; struct sockaddr_storage client_addr;
    while (1){
        clientlen = sizeof(struct sockaddr_storage);
        connfd = malloc(sizeof(int)); //Dynamically allocate space for the fd client can use to communicate w server
        *connfd = accept(listenfd, (struct sockaddr *)&client_addr, &clientlen);
        if (*connfd == -1 && errno != EINVAL) break;
        debug("Connected fd value: %d\n", *connfd);
        pthread_t tid; 
        pthread_create(&tid, NULL, thread, connfd); //Make sure to pass in the fd returned by accept to create
    }
    terminate(EXIT_SUCCESS);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    debug("Shutting down PBX...");
    pbx_shutdown(pbx);
    if (connfd) free(connfd);
    pthread_exit(NULL);
}

/*  
 * open_listenfd - Open and return a listening socket on port. This
 *     function is reentrant and protocol-independent.
 *
 *     On error, returns: 
 *       -2 for getaddrinfo error
 *       -1 with errno set for other errors.
*/
int open_listenfd(char *port) 
{
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval=1;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             /* Accept connections */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* ... on any IP address */
    hints.ai_flags |= AI_NUMERICSERV;            /* ... using port number */
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
        return -2;
    }

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue;  /* Socket failed, try the next */

        /* Eliminates "Address already in use" error from bind */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,    //line:netp:csapp:setsockopt
                   (const void *)&optval , sizeof(int));

        /* Bind the descriptor to the address */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* Success */
        if (close(listenfd) < 0) { /* Bind failed, try the next */
            fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* No address worked */ return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, 1024) < 0) {
        close(listenfd);
	    return -1;
    }
    return listenfd;
}
void *thread(void *vargp){
    pthread_detach(pthread_self());
    return pbx_client_service(vargp);
}