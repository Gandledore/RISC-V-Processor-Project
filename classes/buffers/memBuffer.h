#ifndef MEM_BUFFER_H
#define MEM_BUFFER_H

#include <iostream>
#include <bitset>
#include <mutex>

struct mem_buffer_control_signals{
    bool jump=0;
    bool RegWrite=0;
    bool MemToReg=0;
};
bool operator==(const mem_buffer_control_signals& cs1, const mem_buffer_control_signals& cs2){
    return cs1.jump==cs2.jump && cs1.RegWrite==cs2.RegWrite && cs1.MemToReg==cs2.MemToReg;
}
struct mem_buffer_data{
    mem_buffer_control_signals control_signals;//(Jump, RegWrite, MemToReg) for (write) stage
    int next_pc=0;
    int alu_result=0;
    int write_register=0;
    int mem_read_data=0;
};
std::ostream& operator<<(std::ostream& os, mem_buffer_data& mb){
    os << "Control Signals: " << mb.control_signals.jump << mb.control_signals.RegWrite << mb.control_signals.MemToReg << std::endl;
    os << "Next PC: " << mb.next_pc << std::endl;
    os << "ALU Result: " << mb.alu_result << " | Write Register: " << mb.write_register << " | Mem Read Data: " << mb.mem_read_data;
    return os;
}
#endif