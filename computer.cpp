#include <iostream>
#include <string>
#include "processor.h"

using namespace std;

int main(){
    Processor p(32,32);
    if(p.load("sample_part2.txt")){
        p.run();
    }
    p.print();
    return 0;
}