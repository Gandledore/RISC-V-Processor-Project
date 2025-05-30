#ifndef FETCH_BUFFER_H
#define FETCH_BUFFER_H

#include <iostream>
#include <bitset>
#include <mutex>

struct fetch_buffer_data{
    int instruction=0;
    int pc=0;
};
std::ostream& operator<<(std::ostream& os, fetch_buffer_data& fb){
    os << "Instruction: " << std::bitset<32>(fb.instruction) << " | PC: " << std::hex << fb.pc;
    return os;
}

#endif