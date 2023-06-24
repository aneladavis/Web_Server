/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "networking.h"

#include "thread_pool.h"

#include "clients_statemachine.h"
#include "server_statemachine.h"

void continue_reading_request(struct client *client);
void continue_sending_reply(struct client *client);

// Global variables

struct client *head;
struct client *tail;

// Functions

void init() {
	head = NULL;
	tail = NULL;
}

int insert_client(int socket) {
	struct client *new_client = make_client(socket);

	if(new_client != NULL) {
		new_client->prev = NULL;
		new_client->next = head;

		if(head != NULL) {
			head->prev = new_client;
		}

		head = new_client;

		if(tail == NULL) {
			tail = new_client;
		}

		return 1;
	}

	return 0;
}

struct client *search_client(int socket) {
	struct client *temporary;

	temporary = head;

	while(temporary != NULL) {
		if(socket == temporary->socket) {
			return temporary;
		}

		temporary = temporary->next;
	}

	return NULL;
}

int remove_client(int socket) {
	struct client *found_client = search_client(socket);

	if(found_client == NULL) {
		return 0;
	}

	if(found_client->prev != NULL) {
		found_client->prev->next = found_client->next;
	}
	else {
		head = found_client->next;
	}

	if(found_client->next != NULL) {
		found_client->next->prev = found_client->prev;
	}
	else {
		tail = found_client->prev;
	}

	free(found_client);

	return 1;
}

void handle_client(struct client *client) {
	if(!client) {
		return;
	}

	switch(client->state) {
	case E_RECV_REQUEST:
		continue_reading_request(client);
		break;
	case E_SEND_REPLY:
		continue_sending_reply(client);
		break;
	}
}

void continue_reading_request(struct client *client) {
	int result = read(client->socket, client->buffer + client->nread, BUFFER_SIZE - 1 - client->nread);

	if(result <= 0) {
		fprintf(stderr, "Client socket no. %d closed connection prematurely\n", client->socket);

		client->status = STATUS_BAD;
		finish_client(client);

		return;
	}

	client->nread += result;

	if(header_complete(client->buffer, client->nread)) {
		// If you want to print what's in the response
		printf("Request:\n%s\n", client->buffer);

		char filename[1024];
		char protocol[16];

		if(get_filename(client->buffer, client->nread, filename, 1024, protocol, 16) == -1) {
			fprintf(stderr, "Client socket no. %d sent invalid header - closing connection\n", client->socket);

			client->status = STATUS_BAD;
			finish_client(client);
		}

		switch_state(client, filename, protocol);
	}
}

void continue_sending_reply(struct client *client) {
	int result;

	if(client->ntowrite) {
		result = write(client->socket, client->buffer + client->nwritten, client->ntowrite);

		if(result == -1) {
			client->status = STATUS_BAD;
			finish_client(client);
		}

		client->nwritten += result;
		client->ntowrite -= result;

		return;
	}

	if(client->file == NULL) {
		// If you got here, you're done (in a clean way)
		finish_client(client);

		if(client->status == STATUS_OK) {
			operations_completed++;
		}
	}
	else {
		result = fread(client->buffer, sizeof(char), BUFFER_SIZE, client->file);

		if(result < BUFFER_SIZE) {
			fclose(client->file);
			client->file = NULL;
		}

		client->nwritten = 0;
		client->ntowrite = result;
	}
}

