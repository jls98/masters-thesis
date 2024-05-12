#include <string.h>

#define SHIFT_AMOUNT 3

void victim_loop(){
    char *str="test1";
    for(int i=0;i<1000000;i++){
        asm __volatile__("mfence;");
        i+=2;
        i*=2;
        i-=4;
        i/=2;
    }
    int length = strlen(str);
    for (int i = 0; i < length; ++i) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = 'a' + (str[i] - 'a' + SHIFT_AMOUNT) % 26;
        } else if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = 'A' + (str[i] - 'A' + SHIFT_AMOUNT) % 26;
        }
    }
    for (int i = 0; i < length; ++i) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = 'a' + (str[i] - 'a' + SHIFT_AMOUNT) % 26;
        } else if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = 'A' + (str[i] - 'A' + SHIFT_AMOUNT) % 26;
        }
    }
    for (int i = 0; i < length; ++i) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = 'a' + (str[i] - 'a' + SHIFT_AMOUNT) % 26;
        } else if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = 'A' + (str[i] - 'A' + SHIFT_AMOUNT) % 26;
        }
    }
    for (int i = 0; i < length; ++i) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = 'a' + (str[i] - 'a' + SHIFT_AMOUNT) % 26;
        } else if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = 'A' + (str[i] - 'A' + SHIFT_AMOUNT) % 26;
        }
    }
}

void victim_loop2(){
    char *str="test1";
    for(int i=0;i<1000000;i++){
        asm __volatile__("mfence;");
        i+=2;
        i*=2;
        i-=4;
        i/=2;
    }
    int length = strlen(str);
    for (int i = 0; i < length; ++i) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = 'a' + (str[i] - 'a' + SHIFT_AMOUNT) % 26;
        } else if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = 'A' + (str[i] - 'A' + SHIFT_AMOUNT) % 26;
        }
    }
    for (int i = 0; i < length; ++i) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = 'a' + (str[i] - 'a' + SHIFT_AMOUNT) % 26;
        } else if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = 'A' + (str[i] - 'A' + SHIFT_AMOUNT) % 26;
        }
    }
    for (int i = 0; i < length; ++i) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = 'a' + (str[i] - 'a' + SHIFT_AMOUNT) % 26;
        } else if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = 'A' + (str[i] - 'A' + SHIFT_AMOUNT) % 26;
        }
    }
    for (int i = 0; i < length; ++i) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = 'a' + (str[i] - 'a' + SHIFT_AMOUNT) % 26;
        } else if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = 'A' + (str[i] - 'A' + SHIFT_AMOUNT) % 26;
        }
    }
}

void victim_call0(){
    victim_loop();
    victim_loop();
    victim_loop();
    victim_loop();
}

void victim_call1(){
    victim_loop2();
    victim_loop2();
    victim_loop2();
    victim_loop2();
}

void victim_call2(){
    victim_call1();
    victim_call1();
    victim_call1();
    victim_call1();
}

void victim_call3(){
    victim_call0();
    victim_call0();
    victim_call0();
    victim_call0();
}

void main(){
    victim_call2();
    victim_call2();
    victim_call2();
    victim_call2();
    // victim_call3();
}