CC = gcc
CFLAGS = -Wall -Wextra -masm=intel
all: evict_baseline test_evict_baseline prefetchw

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

prefetchw: prefetch_experiments/prefetchw_characteristics_tests.c
	$(CC) $(CFLAGS) -o build/prefetchw_characteristics_tests  prefetch_experiments/prefetchw_characteristics_tests.c

evict_baseline: Eviction_Set/evict_baseline.c
	$(CC) $(CFLAGS) -o build/evict_baseline -DEVICT_BASELINE Eviction_Set/evict_baseline.c
    
test_evict_baseline: Eviction_Set/test_evict_baseline.c 
	$(CC) $(CFLAGS) -o build/test_evict_baseline -DTEST_EVICT_BASELINE Eviction_Set/test_evict_baseline.c Eviction_Set/evict_baseline.c -lcunit
    
file_generator: utils/file_generator.c
	$(CC) $(CFLAGS) -o build/file_generator utils/file_generator.c
	
pxecute: utils/pxecute.c
	$(CC) $(CFLAGS) -o build/pxecute utils/pxecute.c
	
execute: utils/execute.c
	$(CC) $(CFLAGS) -o build/execute utils/execute.c	


clean:
	rm -f evict_baseline test_evict_baseline prefetchw