#include <stdlib.h>

int victim_loop1(){
    int i= 6;
    i+=2;
    i*=2;
    i-=4;
    i/=2;
    i+=3;
    i*=2;
    i-=6;
    i/=2;
    i+=4;
    i*=2;
    i-=8;
    i/=2;
    i+=3;
    i*=3;
    i-=9;
    i/=3;
    i+=4;
    i*=3;
    i-=12;
    i/=3;
    return i;
    
}

// Garbage code, hopefully blocking prefetching and cache effects.
void barrier(){
    unsigned long int time=0, reps=0;
    void *addr=malloc(8);
    asm __inline__(
        "mov r8, %1;"
        "mov r9, %2;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
        "loop:"		
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
        "mov r8, %1;"
        "mov r9, %2;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
        "mov r8, %1;"
        "mov r9, %2;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
        "mov r8, %1;"
        "mov r9, %2;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
        "mov r8, %1;"
        "mov r9, %2;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
        "mov r8, %1;"
        "mov r9, %2;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
        "mov r8, %1;"
        "mov r9, %2;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
		: "=a" (time)
		: "b" (addr), "r" (reps)
		: "rcx", "rsi", "rdx", "r8", "r9"
    );
}

int victim_loop2(){
    int i=10;
    i+=2;
    i*=2;
    i-=4;
    i/=2;
    i+=3;
    i*=2;
    i-=6;
    i/=2;
    i+=4;
    i*=2;
    i-=8;
    i/=2;
    i+=3;
    i*=3;
    i-=9;
    i/=3;
    i+=4;
    i*=3;
    i-=12;
    i/=3;
    return i;
}

// Garbage code, hopefully blocking prefetching and cache effects.
void barrier2(){
    unsigned long int time=0, reps=0;
    void *addr=malloc(8);
    asm __inline__(
        "mov r8, %1;"
        "mov r9, %2;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
        "loop2:"		
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop2;"
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
        "mov r8, %1;"
        "mov r9, %2;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
        "mov r8, %1;"
        "mov r9, %2;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
        "mov r8, %1;"
        "mov r9, %2;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
        "mov r8, %1;"
        "mov r9, %2;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		"lfence;"
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
        "lfence;"
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
		: "=a" (time)
		: "b" (addr), "r" (reps)
		: "rcx", "rsi", "rdx", "r8", "r9"
    );
}

int main(){
    victim_loop1();
}