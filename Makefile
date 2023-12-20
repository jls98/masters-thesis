CC = gcc
CFLAGS = -Wall -Wextra -masm=intel

all: workshop3 file_generator execute pxecute task1 task2 evict1 jens

workshop3: Cache_Reversing/workshop3.c
	$(CC) $(CFLAGS) -o build/workshop3  Cache_Reversing/workshop3.c
    
task1: Cache_Reversing/task1.c
	$(CC) $(CFLAGS) -o build/task1  Cache_Reversing/task1.c
    
task2: Cache_Reversing/task2.c
	$(CC) $(CFLAGS) -o build/task2  Cache_Reversing/task2.c

evict1: Eviction_Set/evict1.c
	$(CC) $(CFLAGS) -o build/evict1  Eviction_Set/evict1.c
	
jens: Eviction_Set/jens.c
	$(CC) $(CFLAGS) -o build/jens  Eviction_Set/jens.c
    
file_generator: utils/file_generator.c
	$(CC) $(CFLAGS) -o build/file_generator utils/file_generator.c
	
pxecute: utils/pxecute.c
	$(CC) $(CFLAGS) -o build/pxecute utils/pxecute.c
	
execute: utils/execute.c
	$(CC) $(CFLAGS) -o build/execute utils/execute.c	


clean:
	rm -f workshop3 file_generator execute pxecute task1 task2 evict1 jens
