#include <iostream>
#include <string>
#include "classes/processor.h"

using namespace std;

int main(){
    Processor p(32,32);
    if(p.load("test_cases/binary_tests/data_dependency.txt")){
        p.run();
    }
    p.print();
    return 0;
}

// #include <iostream>
// #include <deque>
// #include <algorithm>

// struct instruction_update {
//     int register_destination;
// };

// int main() {
//     std::deque<instruction_update> dependencies = {
//         {1}, {2}  // index 0 (oldest) to 4 (newest)
//     };

//     int target_register = 5;

//     auto it = std::find_if(
//         dependencies.rbegin(), dependencies.rend(),
//         [&](const instruction_update& inst) {
//             return inst.register_destination == target_register && inst.register_destination != 0;
//         });
//     int forward_index = (it==dependencies.rend()) ? dependencies.size() : std::distance(dependencies.begin(), std::prev(it.base()));
//     int offset_from_end = dependencies.size() - forward_index;

//     std::cout << "Found register " << target_register << " at index " << forward_index << "\n";
//     std::cout << "Instruction offset from newest = " << offset_from_end << "\n";

//     return 0;
// }
