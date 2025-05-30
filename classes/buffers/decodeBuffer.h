#ifndef DECODE_BUFFER_H
#define DECODE_BUFFER_H

#include <iostream>
#include <bitset>
#include <mutex>

#include "dependency.h"

struct decode_buffer_control_signals{
    bool jump=0;
    bool RegWrite=0;
    bool MemToReg=0;
    bool MemRead=0;
    bool MemWrite=0;
    int ALUOp=0;
    bool ALUSrc=0;
};
bool operator==(const decode_buffer_control_signals& cs1, const decode_buffer_control_signals& cs2){
    return cs1.jump==cs2.jump && cs1.RegWrite==cs2.RegWrite && cs1.MemToReg==cs2.MemToReg && cs1.MemRead==cs2.MemRead && cs1.MemWrite==cs2.MemWrite && cs1.ALUOp==cs2.ALUOp && cs1.ALUSrc==cs2.ALUSrc;
}
struct decode_buffer_data{
    decode_buffer_control_signals control_signals;//(Jump, RegWrite, MemToReg | MemRead, MemWrite | ALUOp, ALUSrc) for (write | mem | execute) stages
    int next_pc=0;
    dependency dependencies[2];
    int reg1_data=0;
    int reg2_data=0;
    int write_register=0;
    int immediate=0;
    int alu_control=0;
    int reg1=0;
    int reg2=0;
};
std::ostream& operator<<(std::ostream& os, decode_buffer_data& db){
    os << "Control Signals: " << db.control_signals.jump << db.control_signals.RegWrite << db.control_signals.MemToReg << db.control_signals.MemRead << db.control_signals.MemWrite << std::bitset<2>(db.control_signals.ALUOp) << db.control_signals.ALUSrc << std::endl;
    os << "Next PC: " << db.next_pc << std::endl;
    os << "Dependencies: " << db.dependencies[0] << " | " << db.dependencies[1] << std::endl;
    os << "Reg1: " << db.reg1 << " | Reg1 Data: " << db.reg1_data;
    os << "Reg2: " << db.reg2 << " | Reg2 Data: " << db.reg2_data;
    os << "Write Register: " << db.write_register << " | Immediate: " << db.immediate << " | ALU Control: " << std::bitset<4>(db.alu_control);
    return os;
}

#endif
