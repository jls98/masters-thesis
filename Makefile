CC = gcc
CFLAGS = -Wall -Wextra -g -masm=intel
TARGETS = evset

.PHONY: all clean

all: $(TARGETS)

evict_baseline: Eviction_Set/evict_baseline.c
	$(CC) $(CFLAGS) -o build/evict_baseline -DEVICT_BASELINE Eviction_Set/evict_baseline.c
   
test_evict_baseline: Eviction_Set/test_evict_baseline.c 
	$(CC) $(CFLAGS) -o build/test_evict_baseline -DTEST_EVICT_BASELINE Eviction_Set/test_evict_baseline.c Eviction_Set/evict_baseline.c -lcunit

evset: evset.o test_evset.o
    $(CC) $(CFLAGS) $^ -o $@

evset.o: Eviction_Set/evset.c Eviction_Set/evset.h
	$(CC) $(CFLAGS) -c $< -o $@

test_evset.o: Eviction_Set/test_evset.c  Eviction_Set/evset.h
	$(CC) $(CFLAGS) -DNOMAIN -c $< -o $@ -lcunit

#evset: Eviction_Set/evset.c Eviction_Set/evset.h
#	$(CC) $(CFLAGS) -o build/evset Eviction_Set/evset.c

#test_evset: Eviction_Set/test_evset.c  Eviction_Set/evset.h
#	$(CC) $(CFLAGS) -o build/test_evset -DNOMAIN Eviction_Set/test_evset.c Eviction_Set/evset.c -lcunit

pp: PP/pp.c
	$(CC) $(CFLAGS) -g -o build/pp -DPP PP/pp.c

test_pp: PP/test_pp.c 
	$(CC) $(CFLAGS) -g -o build/test_pp -DTEST_PP PP/test_pp.c PP/pp.c -lcunit
    
test_utils: PP/test_utils.c 
	$(CC) $(CFLAGS) -g -o build/test_utils -DTEST_UTILS PP/test_utils.c utils/utils.c -lcunit
    
cache8way: Replacement_Policy/cache8way.c 
	$(CC) $(CFLAGS) -g -o build/cache8way -DREPLACEMENT Replacement_Policy/cache8way.c utils/utils.c
    
clean:
	rm -f $(TARGETS)