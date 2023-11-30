CC = gcc
CFLAGS = -Wall -Wextra -masm=intel

all: workshop3 cache_sizes

workshop3: Cache_Reversing/workshop3.c
	$(CC) $(CFLAGS) -o build/workshop3  Cache_Reversing/workshop3.c
	
cache_sizes: Cache_Reversing/cache_sizes.c
	$(CC) $(CFLAGS) -o build/cache_sizes  Cache_Reversing/cache_sizes.c

file_generator: utils/file_generator.c
	$(CC) $(CFLAGS) -o build/file_generator utils/file_generator.c
	
pxecute: utils/pxecute.c
	$(CC) $(CFLAGS) -o build/pxecute utils/pxecute.c
	
execute: utils/execute.c
	$(CC) $(CFLAGS) -o build/execute utils/execute.c	


clean:
	rm -f workshop3 cache_sizes file_generator execute pxecute
