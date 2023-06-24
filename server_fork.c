/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>

#include "server_fork.h"

#include "clients_common.h"

#include "networking.h"

#include "thread_pool.h"

// Prototypes (forward declarations) for the functions below
void setupSignalHandler(int signal, void (*handler)(int));
void termHandler(int signal);
void childHandler(int signal);

static int done = 0;

int server_fork(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "Usage: server <port>\n");

        return EXIT_FAILURE;
    }

    char *port = argv[1];

    int accept_socket = create_server(atoi(port));

    if(accept_socket == -1) {
        fprintf(stderr, "Error creating server\n");

        return EXIT_FAILURE;
    }

    // Setup signal handlers for SIGPIPE, SIGCHLD, and SIGTERM
    setupSignalHandler(SIGPIPE, SIG_IGN);
    setupSignalHandler(SIGTERM, termHandler);
    setupSignalHandler(SIGCHLD, childHandler);

    // Initialize threads for multithreading
    start_threads();

    while(!done) {
        char host[1024];
        int port;

		int client_socket = accept_client(accept_socket);

		// Wait for new connections, but ignore interrupted accept_client calls because of SIGCHLD
		while(client_socket == -1 && errno == EINTR){
			client_socket = accept_client(accept_socket);
		}

        get_peer_information(client_socket, host, 1024, &port);
        printf("New connection from %s, port %d\n", host, port);

        struct client *client = make_client(client_socket);
        // comment out to use server_fork without multithreading
        put_request(client);

        // // uncomment to use server_fork without multithreading
        // int childID = fork();
        // if(childID == 0) {
		// 	struct client *client = make_client(client_socket);

		// 	if(read_request(client)) {
        //     	write_reply(client);
        // 	}

        //     // save the status of the child 
		// 	int childStatus = client->status;
            
		// 	// Free memory after child process has completed
		// 	free(client);
		// 	return childStatus;
		// }
        // // end uncommenting
	}

    printf("Finishing program cleanly... %ld operations served\n", operations_completed); 
    finish_threads();

    return EXIT_SUCCESS;
}

// Setup signal handlers

void setupSignalHandler(int signal, void (*handler)(int)) {
    struct sigaction options;

    memset(&options, 0, sizeof(struct sigaction));

    options.sa_handler = handler;

    if(sigaction(signal, &options, NULL) == -1) {
        perror("sigaction");

        exit(EXIT_FAILURE);
    }
}

// Collect exit status of child and increase operations completed if STATUS_OK

void childHandler(int signal) {
    int status;
    while(waitpid(-1, &status, WNOHANG) != -1) {
        if(WEXITSTATUS(status)) {
            operations_completed++;
        }
    }
}

// Turn flag on

void termHandler(int signal) {
    done = 1;
}
