/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "clients_common.h"
#include "net/networking.h"

atomic_ulong operations_completed;

int flush_buffer(struct client *client);
int obtain_file_size(char *filename);

struct client *make_client(int socket) {
    struct client *new_client = (struct client *) malloc(sizeof(struct client));

    if(new_client != NULL) {
        // client fd
        new_client->socket = socket;
        new_client->state = E_RECV_REQUEST;

        new_client->file = NULL;

        new_client->nread = 0;
        new_client->nwritten = 0;

        new_client->ntowrite = 0;

        new_client->status = STATUS_OK;

        new_client->prev = NULL;
        new_client->next = NULL;
    }

    return new_client;
}

/**
 *  Reads the HTTP request from the client.
 *  If the header_complete function returns true, then it calls the switch_state function.
 *
 * @param client The client we are writing to.
 *
 * @return If at any point an error is returned by read() or get_filename(), we return 0. Otherwise, we return 1.
 */
int read_request(struct client *client) {
    int bytes_read;
    client->nread = 0;

    // Continue to read in information from the client socket until the header is complete
    do {
        bytes_read = read(client->socket, client->buffer + client->nread, BUFFER_SIZE - 1 - client->nread);
        // increase bytes read if information was read in
        if(bytes_read > 0) {
            client->nread += bytes_read;
        }
        else {
            client->status = STATUS_BAD;
            finish_client(client);
        }
    } while(!header_complete(client->buffer, client->nread) && (client->nread < BUFFER_SIZE));

    char filename[1024];
    char protocol[16];

    // Get filename from client->buffer
    if(get_filename(client->buffer, client->nread, filename, 1024, protocol, 16) == -1) {
        client->status = STATUS_BAD;
        finish_client(client);
        return 0;
    }
    else {
        // filename was read in, call switch state to update status of client
        switch_state(client, filename, protocol);
        // printf("Buffer is: %s", client->buffer);
        return 1;
    }
    return 0;
}
/**
 * Writes HTTP reply to the client
 * Flushes the buffer that contains the header of the response, using flush_buffer
 * Read in information from the buffer
 *
 * @param client The client we are writing to.
 * @param filename The name of the file we are trying to access and open for the client.
 * @param protocol The pointer to the character buffer where the protocol will be stored.
 *
 * @return Nothing.
 */
void switch_state(struct client *client, char *filename, char *protocol) {
    char temporary_buffer[BUFFER_SIZE];

    // Check if the file does not exist
    if(access(filename, F_OK) != 0) {
        get_404(temporary_buffer, filename, protocol);
        client->status = STATUS_404;
    }

    // Check if the file cannot be opened for reading
    client->file = fopen(filename, "r");
    if(client->file == NULL) {
        get_403(temporary_buffer, filename, protocol);
        client->status = STATUS_403;
    }
    // Assume "200 OK" response, The client->status remains STATUS_OK (the default).
    else {
        client->status = STATUS_OK;
        get_200(temporary_buffer, filename, protocol, obtain_file_size(filename));
    }

    // If you want to print what's in the response
    printf("Response:\n%s\n", temporary_buffer);

    strcpy(client->buffer, temporary_buffer);

    // Calculate ntowrite for flush_buffer
    client->ntowrite = strlen(client->buffer);
    client->nwritten = 0;

    write_reply(client);

    client->state = E_SEND_REPLY;
}
/**
 * Flushes the buffer that contains the header of the response, using flush_buffer.
 * Read in information from the buffer.
 * Writes HTTP reply to the client.
 * 
 *
 * @param client The client we are writing to.
 *
 * @return If at any point the flush_buffer return an error, we return 0. Otherwise, we return 1.
 */
int write_reply(struct client *client) {

    if(flush_buffer(client) == 0) {
        client->status = STATUS_BAD;
        finish_client(client);
        return 0;
    }

    // If there is information from the client file, read into the buffer
    if(client->file != NULL) {
        int read_in = fread(client->buffer, sizeof(char), BUFFER_SIZE, client->file);
        // continue to flush the buffer if there is information read in from client file
        while(read_in > 0) {
            client->ntowrite = read_in;
            client->nwritten = 0;
            // sleep(1); // slow down the uploading of the picture
            if(flush_buffer(client) == 0) {
                client->status = STATUS_BAD;
                finish_client(client);
                return 0;
            }

            read_in = fread(client->buffer, sizeof(char), BUFFER_SIZE, client->file);
        }
        return 1;
    }

    finish_client(client);
    return 0;
}

/**
 * Flushes the buffer associated with the client \p client. When this function is called,
 * we keep performing writes until client->ntowrite is 0.
 *
 * Every time we write X bytes, we add X to client->nwritten, and subtract X from client->ntowrite.
 *
 * @param client The client we are writing to.
 *
 * @return If at any point the writes return an error, we return 0. Otherwise, we return 1.
 */
int flush_buffer(struct client *client) {
    int written;
    while(client->ntowrite > 0) {
        written = write(client->socket, client->buffer + client->nwritten, client->ntowrite);
        if(written == -1) {
            perror("Error writing in flush_buffer!");
            return 0;
        }
        client->nwritten += written;
        client->ntowrite -= written;
    }
    return 1;
}

/**
 * Obtains the file size for the filename passed as parameter.
 *
 * @param filename Filename that will have its size returned.
 *
 * @return Size of the filename provided, or -1 if the size cannot be obtained.
 */
int obtain_file_size(char *filename) {
    // Return the size of the 'monsters_inc.jpeg' file. If you try to download anything else form your webserver, you're in trouble...
    struct stat sfile;
    if(stat(filename, &sfile) != 0) {
        return -1;
    }
    return sfile.st_size;
}

/**
 * Closes out a client.
 *
 * @param client The client we are writing to.
 *
 * @return Nothing.
 */
void finish_client(struct client *client) {
    close(client->socket);

    client->socket = -1;
}
