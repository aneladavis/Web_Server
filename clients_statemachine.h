/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause
 */
#ifndef CLIENTS_STATEMACHINE_H
#define CLIENTS_STATEMACHINE_H

#include "clients_common.h"

extern struct client *head;
extern struct client *tail;

void init();

int insert_client(int socket);
struct client *search_client(int socket);
int remove_client(int socket);

void handle_client(struct client *client);

#endif /* CLIENTS_STATEMACHINE_H */
