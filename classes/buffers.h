#include <iostream>
#include <bitset>

struct dependency{
    int buffer_index=0;//which buffer has the correct data
    bool is_dependent=0;
};
std::ostream& operator<<(std::ostream& os, dependency& d){
    os << "Buffer Index: " << d.buffer_index << " | Dependent: " << d.is_dependent;
    return os;
}

struct fetch_buffer{
    int instruction=0;
    int next_pc=0;
};
std::ostream& operator<<(std::ostream& os, fetch_buffer& fb){
    os << "Instruction: " << std::bitset<32>(fb.instruction) << " | Next PC: " << fb.next_pc;
    return os;
}

struct decode_buffer_control_signals{
    bool jump=0;
    bool RegWrite=0;
    bool MemToReg=0;
    bool MemRead=0;
    bool MemWrite=0;
    int ALUOp=0;
    bool ALUSrc=0;
};
struct decode_buffer{
    decode_buffer_control_signals control_signals;//(Jump, RegWrite, MemToReg | MemRead, MemWrite | ALUOp, ALUSrc) for (write | mem | execute) stages
    int next_pc=0;
    dependency dependencies[2];
    int reg1_data=0;
    int reg2_data=0;
    int write_register=0;
    int immediate=0;
    int alu_control=0;
};
std::ostream& operator<<(std::ostream& os, decode_buffer& db){
    os << "Control Signals: " << db.control_signals.jump << db.control_signals.RegWrite << db.control_signals.MemToReg << db.control_signals.MemRead << db.control_signals.MemWrite << std::bitset<2>(db.control_signals.ALUOp) << db.control_signals.ALUSrc << std::endl;
    os << "Next PC: " << db.next_pc << std::endl;
    os << "Dependencies: " << db.dependencies[0] << " | " << db.dependencies[1] << std::endl;
    os << "Reg1 Data: " << db.reg1_data << "Reg2 Data: " << db.reg2_data<< " | Write Register: " << db.write_register << " | Immediate: " << db.immediate << " | ALU Control: " << std::bitset<4>(db.alu_control);
    return os;
}

struct execute_buffer_control_signals{
    bool jump=0;
    bool RegWrite=0;
    bool MemToReg=0;
    bool MemRead=0;
    bool MemWrite=0;
};
struct execute_buffer{
    execute_buffer_control_signals control_signals;//(Jump, RegWrite, MemToReg | MemRead, MemWrite) for (write | mem) stages
    int next_pc=0;
    dependency dependencies[2];
    int alu_result=0;
    int write_register=0;
    int reg2_data=0;
};
std::ostream& operator<<(std::ostream& os, execute_buffer& eb){
    os << "Control Signals: " << eb.control_signals.jump << eb.control_signals.RegWrite << eb.control_signals.MemToReg << eb.control_signals.MemRead << eb.control_signals.MemWrite << std::endl;
    os << "Next PC: " << eb.next_pc << std::endl;
    os << "Dependencies: " << eb.dependencies[0] << " | " << eb.dependencies[1] << std::endl;
    os << "ALU Result: " << eb.alu_result << " | Reg2 Data: " << eb.reg2_data << " | Write Register: " << eb.write_register;
    return os;
}

struct mem_buffer_control_signals{
    bool jump=0;
    bool RegWrite=0;
    bool MemToReg=0;
};
struct mem_buffer{
    mem_buffer_control_signals control_signals;//(Jump, RegWrite, MemToReg) for (write) stage
    int next_pc=0;
    int alu_result=0;
    int write_register=0;
    int mem_read_data=0;
};
std::ostream& operator<<(std::ostream& os, mem_buffer& mb){
    os << "Control Signals: " << mb.control_signals.jump << mb.control_signals.RegWrite << mb.control_signals.MemToReg << std::endl;
    os << "Next PC: " << mb.next_pc << std::endl;
    os << "ALU Result: " << mb.alu_result << " | Write Register: " << mb.write_register << " | Mem Read Data: " << mb.mem_read_data;
    return os;
}