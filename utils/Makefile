CC = gcc
CFLAGS = -Wall -Wextra -masm=intel

all: file_generator execute pxecute

file_generator: file_generator.c
	$(CC) $(CFLAGS) -o file_generator file_generator.c
	
pxecute: pxecute.c
	$(CC) $(CFLAGS) -o pxecute pxecute.c
	
execute: execute.c
	$(CC) $(CFLAGS) -o execute execute.c

clean:
	rm -f file_generator execute pxecute
