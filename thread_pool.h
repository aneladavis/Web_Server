/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause
 */
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#define NUM_THREADS 16

struct request {
	struct client *client;

	struct request *next;
};

int start_threads(void);
int finish_threads(void);
void *handle_clients(void * data);

void put_request(struct client *client);

#endif /* THREAD_POOL_H */
