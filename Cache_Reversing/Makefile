CC = gcc
CFLAGS = -Wall -Wextra -masm=intel

all: workshop3 cache_sizes

workshop3: workshop3.c
	$(CC) $(CFLAGS) -o workshop3  workshop3.c
	
cache_sizes: cache_sizes.c
	$(CC) $(CFLAGS) -o cache_sizes  cache_sizes.c
	


clean:
	rm -f workshop3 cache_sizes
