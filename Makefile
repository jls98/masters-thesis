CC = gcc
CFLAGS = -Wall -Wextra -masm=intel -O0 -fPIC -g
TARGETS = prime_probe evict_time victim2 test_evict_time

all: $(TARGETS)

# maybe not at all? just lib like?
find_evset: src/chap_3_3_find_evset.c
	$(CC) $(CFLAGS) -o build/evset -DEVSETMAIN src/chap_3_3_find_evset.c

test_evset: src/chap_3_3_test_evset.c
	$(CC) $(CFLAGS) -o build/test_evset src/chap_3_3_test_evset.c src/chap_3_3_find_evset.c -lcunit

evict_time: src/chap_4_0_evict_time.c
	$(CC) $(CFLAGS) -o build/evict_time src/chap_4_0_evict_time.c src/chap_3_3_find_evset.c

prime_probe: src/chap_4_1_prime_probe.c
	$(CC) $(CFLAGS) -o build/prime_probe src/chap_4_1_prime_probe.c src/chap_3_3_find_evset.c

test_evict_time: src/chap_5_0_test_evict_time.c
	$(CC) $(CFLAGS) -o build/test_evict_time src/chap_5_0_test_evict_time.c


# file_generator: utils/file_generator.c
#	# $(CC) $(CFLAGS) -o build/file_generator utils/file_generator.c

# evict_baseline: Eviction_Set/evict_baseline.c
#	# $(CC) $(CFLAGS) -o build/evict_baseline -DEVICT_BASELINE Eviction_Set/evict_baseline.c
   
# test_evict_baseline: Eviction_Set/test_evict_baseline.c 
#	# $(CC) $(CFLAGS) -o build/test_evict_baseline -DTEST_EVICT_BASELINE Eviction_Set/test_evict_baseline.c Eviction_Set/evict_baseline.c -lcunit

# evset: Eviction_Set/evset.c
# 	$(CC) $(CFLAGS) -o build/evset Eviction_Set/evset.c
    
# evset-timings: Eviction_Set/evset-timings.c
# 	$(CC) $(CFLAGS) -o build/evset-timings Eviction_Set/evset-timings.c

# test_evset-timings: Eviction_Set/test_evset-timings.c
# 	$(CC) $(CFLAGS) -o build/test_evset-timings -DNOMAIN Eviction_Set/test_evset-timings.c Eviction_Set/evset-timings.c -lcunit

# attacker_evset-timings: Eviction_Set/attacker_evset-timings.c
# 	$(CC) $(CFLAGS) -o build/attacker_evset-timings -DNOMAIN Eviction_Set/attacker_evset-timings.c Eviction_Set/evset-timings.c

victim: Eviction_Set/victim.c 
	$(CC) $(CFLAGS) -o build/victim Eviction_Set/victim.c

victim2: Eviction_Set/victim2.c 
	$(CC) $(CFLAGS) -o build/victim2 -DNOMAIN Eviction_Set/victim2.c

# memory_management: Experiments/memory_management.c
# 	$(CC) $(CFLAGS) -o build/memory_management Experiments/memory_management.c
# test_evset: Eviction_Set/test_evset.c
# 	$(CC) $(CFLAGS) -o build/test_evset -DNOMAIN Eviction_Set/test_evset.c Eviction_Set/evset.c -lcunit

# workshop3: Cache_Reversing/workshop3.c
# 	$(CC) $(CFLAGS) -o build/workshop3 Cache_Reversing/workshop3.c
    
# task2: Cache_Reversing/task2.c
# 	$(CC) $(CFLAGS) -o build/task2 Cache_Reversing/task2.c

# task1: Cache_Reversing/task1.c
# 	$(CC) $(CFLAGS) -o build/task1 Cache_Reversing/task1.c

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