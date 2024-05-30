CC = gcc
CFLAGS = -Wall -Wextra -masm=intel -O0 -fPIC
TARGETS = cache_size cache_line_size cache_ways_sets

all: $(TARGETS)




cache_size: src/chap_3_0_cache_size.c
	$(CC) $(CFLAGS) -o build/cache_size src/chap_3_0_cache_size.c

cache_line_size: src/chap_3_1_cache_line_size.c
	$(CC) $(CFLAGS) -o build/cache_line_size src/chap_3_1_cache_line_size.c

cache_ways_sets: src/chap_3_2_cache_sets_ways.c
	$(CC) $(CFLAGS) -o build/cache_ways_sets src/chap_3_2_cache_sets_ways.c
	
# file_generator: utils/file_generator.c
#	# $(CC) $(CFLAGS) -o build/file_generator utils/file_generator.c

# evict_baseline: Eviction_Set/evict_baseline.c
#	# $(CC) $(CFLAGS) -o build/evict_baseline -DEVICT_BASELINE Eviction_Set/evict_baseline.c
   
# test_evict_baseline: Eviction_Set/test_evict_baseline.c 
#	# $(CC) $(CFLAGS) -o build/test_evict_baseline -DTEST_EVICT_BASELINE Eviction_Set/test_evict_baseline.c Eviction_Set/evict_baseline.c -lcunit

# evset: Eviction_Set/evset.c
# 	$(CC) $(CFLAGS) -o build/evset Eviction_Set/evset.c
    
evset-timings: Eviction_Set/evset-timings.c
	$(CC) $(CFLAGS) -o build/evset-timings Eviction_Set/evset-timings.c

test_evset-timings: Eviction_Set/test_evset-timings.c
	$(CC) $(CFLAGS) -o build/test_evset-timings -DNOMAIN Eviction_Set/test_evset-timings.c Eviction_Set/evset-timings.c -lcunit

attacker_evset-timings: Eviction_Set/attacker_evset-timings.c
	$(CC) $(CFLAGS) -o build/attacker_evset-timings -DNOMAIN Eviction_Set/attacker_evset-timings.c Eviction_Set/evset-timings.c

victim: Eviction_Set/victim.c 
	$(CC) $(CFLAGS) -o build/victim Eviction_Set/victim.c

victim2: Eviction_Set/victim2.c 
	$(CC) $(CFLAGS) -o build/victim2 -DNOMAIN Eviction_Set/victim2.c

memory_management: Experiments/memory_management.c
	$(CC) $(CFLAGS) -o build/memory_management Experiments/memory_management.c
# test_evset: Eviction_Set/test_evset.c
# 	$(CC) $(CFLAGS) -o build/test_evset -DNOMAIN Eviction_Set/test_evset.c Eviction_Set/evset.c -lcunit

# workshop3: Cache_Reversing/workshop3.c
# 	$(CC) $(CFLAGS) -o build/workshop3 Cache_Reversing/workshop3.c
    
task2: Cache_Reversing/task2.c
	$(CC) $(CFLAGS) -o build/task2 Cache_Reversing/task2.c

task1: Cache_Reversing/task1.c
	$(CC) $(CFLAGS) -o build/task1 Cache_Reversing/task1.c

size_jens: Cache_Reversing/size_jens.c
	$(CC) $(CFLAGS) -o build/size_jens Cache_Reversing/size_jens.c

size_jens: Cache_Reversing/size_jens.c
	$(CC) $(CFLAGS) -o build/size_jens Cache_Reversing/size_jens.c



# file_generator: utils/file_generator.c
# 	$(CC) $(CFLAGS) -o build/file_generator utils/file_generator.c

# pp: PP/pp.c
#	# $(CC) $(CFLAGS) -g -o build/pp -DPP PP/pp.c

# test_pp: PP/test_pp.c 
#	# $(CC) $(CFLAGS) -g -o build/test_pp -DTEST_PP PP/test_pp.c PP/pp.c -lcunit
    
# test_utils: PP/test_utils.c 
#	# $(CC) $(CFLAGS) -g -o build/test_utils -DTEST_UTILS PP/test_utils.c utils/utils.c -lcunit
    
# cache8way: Replacement_Policy/cache8way.c 
#	# $(CC) $(CFLAGS) -g -o build/cache8way -DREPLACEMENT Replacement_Policy/cache8way.c utils/utils.c
    
clean:
	rm -f $(TARGETS)
