#include <iostream>
#include <string>
#include "processor.h"

using namespace std;

int main(){
    Processor p("sample_part1.txt");
    p.run();
    return 0;
}