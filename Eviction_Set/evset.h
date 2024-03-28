#ifndef evset
#define evset

#define uint64_t u64

// Utils #################################################
/* wait for cycles cycles and activate cache            */
inline void wait(u64 cycles);

inline void flush(void *adrs);

inline void access(void *adrs);

inline u64 rdtscpfence();

// Node functions ########################################
// TODO
inline void list_init();
inline void list_push(Node **head, Node *e);
inline void list_append(Node **head, Node *e);
inline Node *list_pop(Node **head);
// TODO
inline Node *list_union(Node* list1, Node* list2);
inline Node *list_get(Node **head, u64 *index);
inline Node *list_take(Node **head, u64 *index);

//Config functions #######################################
/* init Config */
inline Config *initConfig(u64 ways,	u64 cache_line_size, u64 threshold, u64 cache_size, u64 test_reps, u64 hugepages);

/* configure Config */
inline void updateConfig(Config *conf, u64 ways, u64 cache_line_size, u64 threshold, u64 cache_size, u64 test_reps, u64 hugepages);


// PRG ###################################################
/* create random seed.                                  */
inline u64 lfsr_create(void);
    
/* get pseudo randomly generated number.                */
inline u64 lfsr_rand(u64* lfsr);

/* helper function for lfsr_rand to shift lfsr.         */
inline u64 lfsr_step(u64 lfsr);


// algorithms ############################################
inline void traverse_list(u64 *addr, u64 size);

inline Node *init_evset(void **candidate_set, u64 candidate_set_size, Node* evict_set, void *target_adrs, Config *conf);

inline Node *find_evset(/* TODO */);

inline int64_t test(void *addr, u64 size, void *target_adrs);









#endif