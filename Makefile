CC = gcc
CFLAGS = -Wall -Wextra -masm=intel
all: pp test_pp

workshop3: Cache_Reversing/workshop3.c
	$(CC) $(CFLAGS) -o build/workshop3  Cache_Reversing/workshop3.c
    
task1: Cache_Reversing/task1.c
	$(CC) $(CFLAGS) -o build/task1  Cache_Reversing/task1.c
    
task2: Cache_Reversing/task2.c
	$(CC) $(CFLAGS) -o build/task2  Cache_Reversing/task2.c

evict1: Eviction_Set/evict1.c
	$(CC) $(CFLAGS) -o build/evict1  Eviction_Set/evict1.c
	
evict2: Eviction_Set/evict2.c
	$(CC) $(CFLAGS) -o build/evict2  Eviction_Set/evict2.c

evict_baseline: Eviction_Set/evict_baseline.c
	$(CC) $(CFLAGS) -o build/evict_baseline -DEVICT_BASELINE Eviction_Set/evict_baseline.c
    
test_evict_baseline: Eviction_Set/test_evict_baseline.c 
	$(CC) $(CFLAGS) -o build/test_evict_baseline -DTEST_EVICT_BASELINE Eviction_Set/test_evict_baseline.c Eviction_Set/evict_baseline.c -lcunit
    
file_generator: utils/file_generator.c
	$(CC) $(CFLAGS) -o build/file_generator utils/file_generator.c

pp: PP/pp.c
	$(CC) $(CFLAGS) -o build/pp -DPP PP/pp.c

test_pp: PP/test_pp.c 
	$(CC) $(CFLAGS) -o build/test_pp -DTEST_PP PP/test_pp.c PP/pp.c -lcunit
    
clean:
	rm -f pp test_pp