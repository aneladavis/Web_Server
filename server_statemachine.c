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

#include "server_statemachine.h"

#include "clients_statemachine.h"

#include "networking.h"

#include "thread_pool.h"

static void setupSignalHandler(int signal, void (*handler)(int));
static void handle_termination(int signal);
static void termHandler(int signal);

static int done = 0;
static int doneTerm = 0;

int server_statemachine(int argc, char **argv) {
    // Syscall results
    int result;
    fd_set set_read;
    fd_set set_write;
    struct timeval tv;

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

    make_nonblocking(accept_socket, 1);

    // Treat signals
    setupSignalHandler(SIGPIPE, SIG_IGN);
    setupSignalHandler(SIGTERM, termHandler);

    // Start linked list of clients
    init();

    struct client *current;


    while(!done) {
        // Zero read and write sets
        FD_ZERO(&set_read);
        FD_ZERO(&set_write);

        // Add the accept socket to the read set
        FD_SET(accept_socket, &set_read);
        int maximum_descriptor = accept_socket;

        // Check if client state is E_RECV_REQUEST, add the client’s socket into the read set.
        // Otherwise, if the client has state E_SEND_REPLY, add the client to the write set.
        for(current = head; current != NULL; current = current->next) {
            if(current->state == E_RECV_REQUEST) {
                FD_SET(current->socket, &set_read);
            }
            else if (current->state == E_SEND_REPLY){
                FD_SET(current->socket, &set_write);
            }

            // find max descriptor
            if(current->socket > maximum_descriptor) {
                maximum_descriptor = current->socket;
                printf("%d", maximum_descriptor);
            }
        }

        // Wait for five seconds
        //tv.tv_sec = 0;
        //tv.tv_usec = 0;

        result = select(maximum_descriptor + 1, &set_read, &set_write, NULL, NULL);

        if(result == -1) {
            perror("select()");
            return 0;
        }
        else if(result == 0) {
            // printf("No data within five seconds.");
            return 0;
        }

        // Test if the accept socket has been flagged ready for reading
        if(FD_ISSET(accept_socket, &set_read)) {
            char host[1024];
            int port;

            int client_socket = accept_client(accept_socket);
            get_peer_information(client_socket, host, 1024, &port);
            printf("New connection from %s, port %d\n", host, port);

            make_nonblocking(client_socket, 1);

            // Inserts the client into the list
            insert_client(client_socket);
        }
        else {
            printf("FD_ISSET returned 0, accept_socket is not ready for reading");
        }

        // Check if any client is in read or write set
        for(current = head; current != NULL; current = current->next) {
            if(FD_ISSET(current->socket, &set_read) || FD_ISSET(current->socket, &set_write)) {
                handle_client(current);
            }
        }

        // Remove dead clients after we process them above
        while(remove_client(-1)) {
        };
    }

    printf("Finishing program cleanly... %ld operations served\n", operations_completed);

    // If we are here, we got a termination signal
    // Go over all clients and close their sockets

    for(current = head; current != NULL; current = current->next) {
        close(current->socket);
    }

    return EXIT_SUCCESS;
}

void setupSignalHandler(int signal, void (*handler)(int)) {
    struct sigaction options;

    memset(&options, 0, sizeof(struct sigaction));

    options.sa_handler = handler;

    if(sigaction(signal, &options, NULL) == -1) {
        perror("sigaction");

        exit(EXIT_FAILURE);
    }
}

void handle_termination(int signal) {
    done = 1;
}

void termHandler(int signal) {
    doneTerm = 1;
}
