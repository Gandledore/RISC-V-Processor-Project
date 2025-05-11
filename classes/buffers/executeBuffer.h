#ifndef EXECUTE_BUFFER_H
#define EXECUTE_BUFFER_H

#include <iostream>
#include <bitset>
#include <mutex>

#include "dependency.h"

struct execute_buffer_control_signals{
    bool jump=0;
    bool RegWrite=0;
    bool MemToReg=0;
    bool MemRead=0;
    bool MemWrite=0;
};
bool operator==(const execute_buffer_control_signals& cs1, const execute_buffer_control_signals& cs2){
    return cs1.jump==cs2.jump && cs1.RegWrite==cs2.RegWrite && cs1.MemToReg==cs2.MemToReg && cs1.MemRead==cs2.MemRead && cs1.MemWrite==cs2.MemWrite;
}
struct execute_buffer_data{
    execute_buffer_control_signals control_signals;//(Jump, RegWrite, MemToReg | MemRead, MemWrite) for (write | mem) stages
    int next_pc=0;
    dependency dependencies[2];
    int alu_result=0;
    int write_register=0;
    int reg2_data=0;
    int reg2=0;
};
std::ostream& operator<<(std::ostream& os, execute_buffer_data& eb){
    os << "Control Signals: " << eb.control_signals.jump << eb.control_signals.RegWrite << eb.control_signals.MemToReg << eb.control_signals.MemRead << eb.control_signals.MemWrite << std::endl;
    os << "Next PC: " << eb.next_pc << std::endl;
    os << "Dependencies: " << eb.dependencies[0] << " | " << eb.dependencies[1] << std::endl;
    os << "ALU Result: " << eb.alu_result << " | Reg2: " << eb.reg2 << " | Reg2 Data: " << eb.reg2_data << " | Write Register: " << eb.write_register;
    return os;
}

#endif