void victim_loop1(){
    int i=5;
    i+=2;
    i*=2;
    i-=4;
    i/=2;
    
}

void victim_loop2(){
    int i=5;
    i+=2;
    i*=2;
    i-=4;
    i/=2;
}

// void victim_call0(){
//     victim_loop();
//     victim_loop();
//     victim_loop();
//     victim_loop();
// }

// void victim_call1(){
//     victim_loop2();
//     victim_loop2();
//     victim_loop2();
//     victim_loop2();
// }

// void victim_call2(){
//     victim_call1();
//     victim_call1();
//     victim_call1();
//     victim_call1();
// }

// void victim_call3(){
//     victim_call0();
//     victim_call0();
//     victim_call0();
//     victim_call0();
// }

int main(){
    victim_loop1();
    // victim_call3();
}