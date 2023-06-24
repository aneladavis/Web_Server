/*
 * Copyright (c) 2018, Hammurabi Mendes.
 * License: BSD 2-clause
 */
#ifndef NETWORKING_H
#define NETWORKING_H

/**
 * Create an accept socket in the specified \p port.
 *
 * @param port Port where the server should run.
 * @return The server socket, or -1 if an error is found -- errno indicate the error.
 */
int create_server(int port);

/**
 * Accept a client in the specified \p accept_socket.
 *
 * @param accept_socket Accept socket used to listen to clients.
 * @return The client socket, or -1 if an error is found -- errno indicate the error.
 */
int accept_client(int accept_socket);

/**
 * Get information about the peer connected to the socket \p socket.
 *
 * @param socket Socket that is connected to the peer we are getting information from.
 * @param host_string Pointer to the character buffer that will hold the hostname.
 * @param host_length Length of the character buffer that will hold the hostname.
 * @param port Pointer to an integer that will contain the port upon return.
 */
void get_peer_information(int socket, char *host_string, int host_length, int *port);

/**
 * Turns socket non-blocking on/off.
 * When non-blocking is on, read and write operations will return error codes instead of blocking.
 * When non-blocking is off... well, it blocks! :)
 *
 * @param socket Socket that will have its non-blocking flag set on/off.
 * @param flag 1 to turn non-blockingness ON; 0 to turn it OFF.
 */
void make_nonblocking(int socket, int flag);

/**
 * Create a connected socket in the specified \p destination and \p port.
 *
 * @param destination Destination where we should connect to.
 * @param port Port where we should connect to.
 * @return The connected socket, or -1 if an error is found -- errno indicate the error.
 */
int create_client(char *destination, char *port);

/**
 * Returns true if the buffer contains a complete HTTP request header.
 *
 * NOTE: This function will add a null character to the end of the buffer, so make sure that it contains
 *       at least one extra byte available for that.
 *
 * @param buffer Pointer to the character buffer where data is being received.
 * @param buffer_length Length of the character buffer where data is being received.
 *
 * @return 1 if the header is complete; 0 otherwise.
 */
int header_complete(char *buffer, int buffer_length);

/**
 * Parses a filename out of the HTTP header (assuming it is complete - otherwise the behaviour is undefined).
 *
 * @param buffer Pointer to the buffer containing a complete (and null-terminated) HTTP header.
 * @param buffer_length Length of the buffer containing a complete (and null-terminated) HTTP header.
 * @param filename Pointer to the character buffer where the filename will be stored.
 * @param filename_length Length of the character buffer where the filename will be stored.
 * @param protocol Pointer to the character buffer where the protocol will be stored.
 * @param protocol_length Length of the character buffer where the protocol will be stored.
 *
 * @return 0 if the filename/protocol have been parsed successfully; -1 otherwise.
 */
int get_filename(char *buffer, int buffer_length, char *filename, int filename_length, char *protocol, int protocol_length);

/**
 * @param buffer Pointer to the buffer where the HTTP response will be written (assumed to be BUFFER_SIZE of length).
 * @param filename Pointer to the character buffer containing the filename requested by the client.
 * @param protocol Pointer to the character buffer containign the protocol used by the client.
 * @param filesize Length of the file requested by the client.
 */
void get_200(char *buffer, char *filename, char *protocol, int filesize);

/**
 * @param buffer Pointer to the buffer where the HTTP response will be written (assumed to be BUFFER_SIZE of length).
 * @param filename Pointer to the character buffer containing the filename requested by the client (but not accessible).
 * @param protocol Pointer to the character buffer containign the protocol used by the client.
 */
void get_403(char *buffer, char *filename, char *protocol);

/**
 * @param buffer Pointer to the buffer where the HTTP response will be written (assumed to be BUFFER_SIZE of length).
 * @param filename Pointer to the character buffer containing the filename requested by the client (but not existent).
 * @param protocol Pointer to the character buffer containign the protocol used by the client.
 */
void get_404(char *buffer, char *filename, char *protocol);

#endif /* NETWORKING_H */
