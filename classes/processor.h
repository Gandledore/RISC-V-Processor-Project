#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <bitset>
#include <iomanip>
#include "instruction.h"
#include "buffers.h"

int string_to_binary(std::string num){
    int digit = 0;
    for (int i = 0; i < num.length(); i++){
        digit = digit << 1;         //bit shifts left to make room for next bit
        digit += (num[i] == '1');//adds a 1 to the end of the binary number if it is a 1, otherwise its a 0 so it adds 0
    }
    return digit;
}
struct ControlSignals {
    bool jump;
    bool RegWrite;
    bool MemToReg;
    bool MemRead;
    bool MemWrite;
    int ALUOp;
    bool ALUSrc;
    bool Branch;
};
std::ostream& operator<<(std::ostream& os,ControlSignals& cs){                                                 //allows printing
    os << cs.ALUSrc << cs.MemToReg << cs.RegWrite << cs.MemRead << cs.MemWrite << cs.Branch << cs.ALUOp;
    return os;
}

class Processor{
    //program counter
    int pc;
    bool flush_flag;
    //data
    int rf[32];
    int* dmem;
    int* imem;
    int imem_size;
    int dmem_size;

    //buffers for variables used between pipeline stages
    fetch_buffer fetch_buff;
    decode_buffer decode_buff;
    execute_buffer execute_buff;
    mem_buffer mem_buff;

    int total_clock_cycles;
    bool program_loaded;
    int max_pc;//last instruction address + sizeof(int)

    public:
    Processor(int imem_size = 32, int dmem_size=32){//set up instruction memory, data memory, and registers, and default tracking vars (pc, control_signals, total_clock_cycles, program_loaded)
        this->imem_size = imem_size;
        this->dmem_size = dmem_size;
        dmem = new int[imem_size];
        imem = new int[dmem_size];

        //set up registers and memory to 0
        memset(rf, 0, sizeof(rf));
        memset(dmem, 0, dmem_size * sizeof(int));
        memset(imem, 0, imem_size * sizeof(int));

        //sample_1 initial state
        // rf[1]=0x20; rf[2]=0x5;  rf[10]=0x70;    rf[11]=0x4;
        // dmem[0x70/sizeof(int)]=0x5;   dmem[0x74/sizeof(int)]=0x10;

        //sample_2 initial state
        // rf[8]=0x20; rf[10]=0x5; rf[11]=0x2; rf[12]=0xa; rf[13]=0xf;

        //test initial state: expected output: 0x14 at memory 0x5c 
        // dmem[3]=1; dmem[4]=-3; dmem[5]=5; dmem[6]=-2; dmem[7]=19;

        //test2 initial state: expected output: 0x20 at memory 0x64
        // dmem[5]=0xabc; dmem[6]=0xdef; dmem[7]=0x1234; dmem[8]=0x5678; dmem[9]=0x9abc;
        // std::cout << std::hex << (dmem[5] & dmem[6] & dmem[7] & dmem[8] & dmem[9]) << std::endl;
        
        //test3 initial state: expected output 0x27a at memory 0x64
        dmem[3] = 0x34; dmem[4] = 0x77; dmem[5] = 0x28; dmem[6] = 0xb6; dmem[7] = 0xf1;

        //tracking variables
        pc = 0;
        flush_flag = false;
        total_clock_cycles = 0;
        program_loaded = false;
    }

    ~Processor(){//destructor
        delete[] dmem;
        delete[] imem;
    }
    
    bool load(std::string filepath){//parse textfile, store to imem, return bool for success status
        std::ifstream file(filepath);
        if (!file) {
            std::cerr << "Error opening file!" << std::endl;
            return false; //failure
        }
        std::string line;
        int i=0;
        while(i<imem_size && std::getline(file, line)){
            line = line.back()=='\r' ? line.substr(0, line.length()-1) : line; //remove \r if present (windows)
            //i++ operator increments i after the value is used
            imem[i++] = string_to_binary(line);//convert string to binary and store in instruction memory
        }
        file.close();
        max_pc=i*sizeof(int);//last instruction address + sizeof(int)
        program_loaded=true;
        return true;//succes
    }
    
    void print() {//prints instruction memory, register file, and data memory (written by chatGPT, edited by Ryan)
        std::cout << "\n  Addr:  Instruction Memory (Binary)     | Reg (hex) | Data Memory (hex)" << std::endl;
        std::cout << "-------------------------------------------------------------------------" << std::endl;
    
        int max_size = std::max(imem_size, std::max(32, dmem_size));
        for (int i = 0; i < max_size; i++) {
            // Instruction Memory
            if (i < imem_size) {
                std::cout << "0x" << std::hex << std::setw(4) << i * sizeof(int) << ": " << std::bitset<32>(imem[i]) << " | ";
            } else {std::cout << std::setw(43) << " | ";}  // alignment
    
            // Registers
            if (i < 32) {
                std::cout << "x" << std::dec << std::setw(2) << i << ": " << std::setw(4) << std::hex << rf[i] << " | ";
            } else {std::cout << "          | ";}  //alignment
    
            // Data Memory
            if (i < dmem_size) {
                std::cout << "0x" << std::hex << std::setw(4) << i * sizeof(int) << ": " << std::setw(4) << dmem[i];
            }
    
            std::cout << std::endl;
        }
    }

    void run(){//run loaded program
        if(!program_loaded){
            std::cout << "No program loaded" << std::endl;
            return;
        }
        int inst;
        int instruction_count = 0;
        int max_instructions = 100;
        while(pc<max_pc && pc>=0 && instruction_count++<max_instructions){//within bounds of instruction memory
            Fetch();
            Decode();
            Execute();
            Mem();
            Write();
            std::cout << "pc is modified to 0x" << std::hex << pc << std::endl;
        }
        if(pc==max_pc){
            std::cout << "\nProgram Finished\n" << std::endl;
        }
        else{
            std::cout << "Instruction Segmentation Fault" << std::endl;
            std::cout << "pc: " << std::hex << pc << std::endl;
        }
        return;
    }

    private:
    void Fetch(){
        fetch_buff.instruction = imem[pc/sizeof(int)];//instruction memory is composed of ints so divide by sizeof(int)
        fetch_buff.next_pc = pc + sizeof(int);
        total_clock_cycles+=1;
        std::cout << "\ntotal_clock_cycles: " << std::dec << total_clock_cycles << std::endl;
        if(flush_flag){//pretend to flush (since happening after decode stage updates the pc value)
            flush_flag = false;
            std::cout<< "Instruction Updated" << std::endl;
        }
    }

    void Decode(){//returns instruction_data type = r1_data,r2_data,write_reg,immediate,alu_ctrl (instr[12-14|30])
        if (flush_flag){//if flush is true, set decode buffer to flush
            std::cout << "Flushing" << std::endl;
            decode_buff.control_signals = {0,0,0,0,0,0b00,0};
            flush_flag = false;
            return;
        }
        int inst = fetch_buff.instruction;
        int next_pc = fetch_buff.next_pc;

        Instruction instruction(inst);
        decoded_data data = instruction.decode();
        
        ControlSignals control_signals = ControlUnit(data.opcode);
        if(data.write_register == 0){//if write register is 0, set to reg write signal to false to avoid writing to it
            control_signals.RegWrite = false;
        }

        //Stall Detection Calculation
        //if stall is needed, set control signals to 0, don't update pc so it fetches the same instruction, return
        
        int reg1_data = rf[data.read_register_1];
        int reg2_data = rf[data.read_register_2];
        //early branch detection
        int branch_target = pc + data.immediate;//calculate branch target adress, instruction offset bit shift accounted for in decoding of immediate
        int jump_target = control_signals.Branch ? branch_target : reg1_data+data.immediate; //jump target mux
        pc = (control_signals.jump || (control_signals.Branch && (reg1_data==reg2_data))) ? jump_target : next_pc;//branch mux
        
        //update decode buffer
        if(pc != next_pc){//if branch not taken, control signals are valid
            flush_flag = true;
            // std::cout << "Marked for flush" << std::endl;
        }
        
        decode_buff.control_signals = {control_signals.jump, 
                                        control_signals.RegWrite, 
                                        control_signals.MemToReg, 
                                        control_signals.MemRead, 
                                        control_signals.MemWrite, 
                                        control_signals.ALUOp, 
                                        control_signals.ALUSrc};
        decode_buff.next_pc = next_pc;
        decode_buff.reg1_data = reg1_data;
        decode_buff.reg2_data = reg2_data;
        decode_buff.write_register = data.write_register;
        decode_buff.immediate = data.immediate;
        decode_buff.alu_control = data.alu_control;
    
        return;
    }
    
    ControlSignals ControlUnit(int opcode){
        //control_signal = {Jump, RegWrite, MemToReg, MemRead, MemWrite, ALUOp, ALUSrc, Branch}
        ControlSignals control_signals = {0,0,0,0,0,0b00,0,0};
        switch(opcode){
            case 0b0110011:
            case 0b0111011://R type
                control_signals = {0,1,0,0,0,0b10,0,0};
                break;
            case 0b0100011://save
                control_signals = {0,0,0,0,1,0b00,1,0};
                break;
            case 0b1100011://branch
                control_signals = {0,0,0,0,0,0b01,0,1};
                break;
            case 0b0000011://load
                control_signals = {0,1,1,1,0,0b00,1,0};
                break;
            case 0b010011://I type special i types not implemented (the ones that use funct7)
                control_signals = {0,1,0,0,0,0b11,1,0};
                break;
            case 0b1101111://jal
                control_signals = {1,1,0,0,0,0b00,0,1};
                break;
            case 0b1100111://jalr
                control_signals = {1,1,0,0,0,0b00,0,0};
                break;
            case 0b0000000://nop
                control_signals = {0,0,0,0,0,0b00,0,0};
                break;
            default:
                std::cout << "Undefined Control Unit Behavior for opcode "<< std::bitset<7>(opcode) << std::endl;
        }
        return control_signals;
    }
    
    enum ALU_Op{nil, ADD, SUB, AND, OR};
    ALU_Op ALU_Control(int alu_op, int alu_ctrl){
        //alu_control = instruction[12-14|30]
        ALU_Op alu_control = nil;
        switch(alu_op){
            case 0b00://lw,sw
                alu_control = ADD;
                break;
            case 0b01://beq
                alu_control = SUB;
                break;
            case 0b11://I type
                alu_ctrl &= 0b1110;//rightmost bit part of immediate, first 3 are funct3
                //no break, fallthrough to R
            case 0b10://R type
                switch(alu_ctrl){
                    case 0b0000:
                        alu_control = ADD;
                        break;
                    case 0b0001:
                        alu_control = SUB;
                        break;
                    case 0b1110:
                        alu_control = AND;
                        break;
                    case 0b1100:
                        alu_control = OR;
                        break;
                    default:
                        std::cout << "Undefined ALU Control Behavior for alu_ctrl "<< std::bitset<4>(alu_ctrl) << std::endl;
                }
                break;
            default:
                std::cout << "Undefined ALU Control Behavior for alu_op "<< std::bitset<2>(alu_op) << std::endl;
        }
        return alu_control;
    }
    
    void Execute(){
        decode_buffer_control_signals control_signals = decode_buff.control_signals;
        int alu_control = decode_buff.alu_control;
        int immediate = decode_buff.immediate;
        //assuming no dependencies
        int reg1_data = decode_buff.reg1_data;
        int reg2_data = decode_buff.reg2_data;
        
        int input2 = control_signals.ALUSrc ? immediate : reg2_data; //ALU_Src mux
        ALU_Op alu_operation = ALU_Control(control_signals.ALUOp, alu_control);
        int alu_result;
        switch(alu_operation){
            case ADD:
                alu_result = reg1_data + input2;
                break;
            case SUB:
                alu_result = reg1_data - input2;
                break;
            case AND:
                alu_result = reg1_data & input2;
                break;
            case OR:
                alu_result = reg1_data | input2;
                break;
            case nil:
                //do nothing (ie stall)
                break;
            default:
                std::cout << "Undefined ALU Behavior for alu_control "<< std::bitset<4>(alu_control) << std::endl;
        }

        //update execute buffer
        execute_buff.control_signals = {control_signals.jump, 
                                        control_signals.RegWrite, 
                                        control_signals.MemToReg, 
                                        control_signals.MemRead, 
                                        control_signals.MemWrite};
        execute_buff.next_pc = decode_buff.next_pc;
        execute_buff.alu_result = alu_result;
        execute_buff.write_register = decode_buff.write_register;
        execute_buff.reg2_data = decode_buff.reg2_data;
        execute_buff.dependencies[0] = decode_buff.dependencies[0];
        execute_buff.dependencies[1] = decode_buff.dependencies[1];
        return;
    }

    void Mem(){
        execute_buffer_control_signals control_signals = execute_buff.control_signals;
        int alu_result = execute_buff.alu_result;
        int reg2_data = execute_buff.reg2_data;
        int next_pc = execute_buff.next_pc;
        int write_reg = execute_buff.write_register;

        int address = alu_result/sizeof(int); //data memory is composed of ints so divide by sizeof(int)
        int mem_read_data;
        if(control_signals.MemRead){
            mem_read_data = dmem[address];
        }
        if(control_signals.MemWrite){
            dmem[address] = reg2_data;
            std::cout << "memory 0x" << std::hex << alu_result << " is modified to 0x" << std::hex << reg2_data << std::endl;
        }

        //update mem buffer
        mem_buff.control_signals = {control_signals.jump, 
                                    control_signals.RegWrite, 
                                    control_signals.MemToReg};
        mem_buff.next_pc = next_pc;
        mem_buff.alu_result = alu_result;
        mem_buff.write_register = write_reg;
        mem_buff.mem_read_data = mem_read_data;
        return;
    }

    void Write(){
        mem_buffer_control_signals control_signals = mem_buff.control_signals;
        int mem_read_data = mem_buff.mem_read_data;
        int alu_result = mem_buff.alu_result;
        int next_pc = mem_buff.next_pc;
        int write_reg = mem_buff.write_register;

        int write_data = control_signals.MemToReg ? mem_read_data : alu_result;     //write back mux
        write_data = control_signals.jump ? next_pc : write_data; //jump mux
        if(control_signals.RegWrite){
            rf[write_reg] = write_data;
            std::cout << "x" << std::dec << write_reg << " is modified to 0x" << std::hex << write_data << std::endl;
        }
        return;
    }
};