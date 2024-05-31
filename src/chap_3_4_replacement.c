#include "chap_3_3_find_evset.c"


int main(){
    u64 ret =106;
    while(ret > 105){
        ret = replacement_L2_only_L2_complete(atoi(av[1]));
    }
    printf("%lu\n", ret);
}