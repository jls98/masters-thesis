CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -masm=intel

all: Zeitmessung1 Zeitmessung2 Zeitmessung3 file_generator execute pxecute

Zeitmessung1: Zeitmessung1.c
	$(CC) $(CFLAGS) -o Zeitmessung1 Zeitmessung1.c

Zeitmessung2: Zeitmessung2.c
	$(CC) $(CFLAGS) -o Zeitmessung2 Zeitmessung2.c
	
Zeitmessung3: Zeitmessung3.c
	$(CC) $(CFLAGS) -o Zeitmessung3 Zeitmessung3.c
	
	
file_generator: file_generator.c
	$(CC) $(CFLAGS) -o file_generator file_generator.c
	
pxecute: pxecute.c
	$(CC) $(CFLAGS) -o pxecute pxecute.c
	
execute: execute.c
	$(CC) $(CFLAGS) -o execute execute.c

clean:
	rm -f Zeitmessung1 Zeitmessung2 Zeitmessung3 file_generator execute pxecute
