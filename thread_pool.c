/*
 * Copyright (c) 2023, Anela Davis.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>

#include "server_statemachine.h"

#include "clients_statemachine.h"

#include "networking.h"

#include "thread_pool.h"

int queue_size = 0;
int th_done = 0;

struct request *request;

pthread_t threadNumber[NUM_THREADS];
pthread_cond_t queue_not_empty;
pthread_mutex_t queue_lock;

static struct request *taskHead;


/**
 * Obtains lock and inserts clients into consumption list for consumers
 * Updates list of consumers and size
 * 
 * @param data not used.
 * @return Nothing.
*/
void put_request(struct client *client) {
    pthread_mutex_lock(&queue_lock);
    queue_size++;
	struct request *current = malloc(sizeof(struct request));
	// Begin at head of list
	struct request *look = request;
	// Initialize the rest of the request
	current->client = client;
	current->client->next = NULL;
	current->next = NULL;
	if(queue_size == 1){
		// Update list if this client is the only request
		taskHead = current;
		request = current;
	}
	else{
		// Move through list until the end
		while(look->client->next != NULL){
			look = look->next;
		}
		// Add in request and update list
		look->client->next = client;
		look->next = current;
	}

    printf("Produced: virtual queue has size %d\n", queue_size);
    pthread_cond_signal(&queue_not_empty);
    pthread_mutex_unlock(&queue_lock);
}

/**
 * Obtains lock and gets the request from the list of requests and completes the client task.
 * 
 * @param data not used.
 * @return Nothing.
*/
void *handle_clients(void * data) {
    while(!th_done) {
		pthread_mutex_lock(&queue_lock);
		// Wait until queue is not empty to consume
		while(queue_size == 0) {
			pthread_cond_wait(&queue_not_empty, &queue_lock);
		}
		queue_size--;
		// Get client 
		struct client *client = taskHead->client;
		// Update head
		taskHead = taskHead->next;
		pthread_mutex_unlock(&queue_lock);
		// Do request
		if(read_request(client)) {
			write_reply(client);
		}
		if(client->status == STATUS_OK) {
			atomic_fetch_add(&operations_completed, 1);
		}
		free(client);
    }
}

/**
 * Launches all the threads
 * 
 * @param none.
 * @return Exit status of function.
*/
int start_threads(){
	/*
		This code is borrowed and modified from pthreads.c by Hammurabi Mendes.
	*/

	// Initialize the mutex
	pthread_mutex_init(&queue_lock, NULL);

	// Initialize the condition variable
	pthread_cond_init(&queue_not_empty, NULL);

	int threadData[NUM_THREADS];

	for(int i = 0; i < NUM_THREADS; i++) {
  	threadData[i] = i;
	}

	pthread_attr_t config;

	// initialize variable 
	pthread_attr_init(&config);
	// go into configuration adn add detached config option
	// detatch operation allows threads to be joinable by waiting until both are finished
	pthread_attr_setdetachstate(&config, PTHREAD_CREATE_JOINABLE);

	// Launches all threads -- they are automatically started
	for(int i = 0; i < NUM_THREADS; i++) {
		if(pthread_create(&threadNumber[i], &config, handle_clients, (void *) &threadData[i]) != 0) {
			perror("pthread_create");

			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

/**
 * Joins all threads and 
 * 
 * @param none.
 * @return Exit status of function.
*/
int finish_threads(){
	/*
		This code is borrowed and modified from pthreads.c by Hammurabi Mendes.
	*/
	// signal that th_done is changed
	pthread_mutex_lock(&queue_lock);
	th_done = 1;
	pthread_mutex_unlock(&queue_lock);

	void *status;

	for(int i = 0; i < NUM_THREADS; i++) {
		if(pthread_join(threadNumber[i], NULL) != 0) {
	
			fprintf(stderr, "Error joning threads\n");

			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}