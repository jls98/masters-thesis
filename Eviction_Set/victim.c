

void victim_loop(){
    for(int i=0;i<1000000;i++){
        asm __volatile__("mfence;");
        i++;
        i*=2;
        i-=2;
        i/=2;
    }
}

void victim_call(){
    victim_loop();
    victim_loop();
    victim_loop();
    victim_loop();
}

void victim_call2(){
    victim_call();
    victim_call();
    victim_call();
    victim_call();
}


void main(){
    victim_call2();
    victim_call2();
    victim_call2();
    victim_call2();
}