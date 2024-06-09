CC = gcc
CFLAGS = -Wall -Wextra -masm=intel -O0 -fPIC -g
TARGETS = prime_probe evict_time test_evict_time victim_new cache_size cache_line_size cache_ways_sets

all: $(TARGETS)

cache_size: src/chap_3_0_cache_size.c
	$(CC) $(CFLAGS) -o build/cache_size src/chap_3_0_cache_size.c

cache_line_size: src/chap_3_1_cache_line_size.c
	$(CC) $(CFLAGS) -o build/cache_line_size src/chap_3_1_cache_line_size.c

cache_ways_sets: src/chap_3_2_cache_sets_ways.c
	$(CC) $(CFLAGS) -o build/cache_ways_sets src/chap_3_2_cache_sets_ways.c
	
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

victim_new: src/victim_new.c 
	$(CC) $(CFLAGS) -o build/victim_new src/victim_new.c

clean:
	rm -f $(TARGETS)
