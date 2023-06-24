/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause
 */
#ifndef CLIENTS_COMMON_H
#define CLIENTS_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#define E_RECV_REQUEST  1
#define E_SEND_REPLY    2

#define STATUS_OK  EXIT_SUCCESS
#define STATUS_BAD EXIT_FAILURE
#define STATUS_403 128 // Standard is to use 128+N for general error N
#define STATUS_404 129 // Standard is to use 128+N for general error N

#define BUFFER_SIZE     4096

struct client {
	int socket;
	int state;

	FILE *file;

	int nread;
	int nwritten;

	int ntowrite;

	char buffer[BUFFER_SIZE];

	int status;

	// These parameters are used in the state machine version

	struct client *prev;
	struct client *next;
};

extern atomic_ulong operations_completed;

struct client *make_client(int socket);

int read_request(struct client *client);

void switch_state(struct client *client, char *filename, char *protocol);

int write_reply(struct client *client);

void finish_client(struct client *client);

#endif /* CLIENTS_COMMON_H */
