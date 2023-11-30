CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -masm=intel

all: Zeitmessung1 Zeitmessung2 execute pxecute

Zeitmessung1: Zeitmessung1.c
	$(CC) $(CFLAGS) -o Zeitmessung1 Zeitmessung1.c
	
Zeitmessung2: Zeitmessung2.c
	$(CC) $(CFLAGS) -o Zeitmessung2 Zeitmessung2.c
	
pxecute: pxecute.c
	$(CC) $(CFLAGS) -o pxecute pxecute.c
	
execute: execute.c
	$(CC) $(CFLAGS) -o execute execute.c

clean:
	rm -f Zeitmessung1 Zeitmessung2 execute pxecute
