#include <iostream>
#include <string>
#include "classes/processor.h"

using namespace std;

int main(){
    Processor p(32,32);
    if(p.load("test_cases/binary_tests/sample_part2.txt")){
        p.run();
    }
    p.print();
    return 0;
}