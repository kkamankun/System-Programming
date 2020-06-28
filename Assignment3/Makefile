CFLAGS = -g

CC = gcc

all: cli srv 

cli: cli.c
	$(CC) -o $@ $^

srv: srv.c
	$(CC) -o $@ $^

clean: 
	rm -rf $@
