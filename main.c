/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause
 */
#include "server_fork.h"
#include "server_statemachine.h"
#include "clients_statemachine.h"

int main(int argc, char **argv) {
	return server_fork(argc, argv);
}