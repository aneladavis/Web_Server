PROGRAM = webserver
OBJECTS = main.o clients_common.o clients_statemachine.o server_fork.o server_statemachine.o thread_pool.o

webserver-clean: clean webserver

webserver: $(OBJECTS)
	clang -g -o webserver $(OBJECTS) -I. -Inet -Lnet -lWildcatNetworking

%.o: %.c
	clang -g -c -o $@ -I. -Inet $<

clean:
	rm -f *.o webserver
