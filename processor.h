#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include "instruction.h"
#include <bitset>
#include <iomanip>

int string_to_binary(std::string num){
    int digit = 0;
    for (int i = 0; i < num.length(); i++){
        digit = digit << 1;         //bit shifts left to make room for next bit
        digit += (num[i] == '1');//adds a 1 to the end of the binary number if it is a 1, otherwise its a 0 so it adds 0
    }
    return digit;
}
struct ControlSignals {
    bool ALUSrc;
    bool MemToReg;
    bool RegWrite;
    bool MemRead;
    bool MemWrite;
    bool Branch;
    int ALUOp;
};
std::ostream& operator<<(std::ostream& os,ControlSignals& cs){                                                 //allows printing
    os << cs.ALUSrc << cs.MemToReg << cs.RegWrite << cs.MemRead << cs.MemWrite << cs.Branch << cs.ALUOp;
    return os;
}

struct decoded_instruction{
    int reg1_data;
    int reg2_data;
    int write_register;
    int immediate;
    int alu_control;
};
std::ostream& operator<<(std::ostream& os,decoded_instruction& di){                                                 //allows printing
    os << "r1: " << di.reg1_data << " | r2: " << di.reg2_data << " | wr: " << di.write_register << " | imm: " << di.immediate << " | alu: " << std::bitset<4>(di.alu_control);
    return os;
}

class Processor{
    //program counter
    int pc;
    int next_pc;
    ControlSignals control_signals;
    //data
    int rf[32];
    int* dmem;
    int* imem;
    int imem_size;
    int dmem_size;
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
        rf[1]=0x20; rf[2]=0x5;  rf[10]=0x70;    rf[11]=0x4;
        dmem[0x70/sizeof(int)]=0x5;   dmem[0x74/sizeof(int)]=0x10;

        //tracking variables
        pc = 0;
        control_signals = {0,0,0,0,0,0,0};
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
        decoded_instruction instruction_data;
        while(pc<max_pc && pc>=0){//within bounds of instruction memory
            inst = Fetch();
            instruction_data = Decode(inst);
            int alu_result = Execute(instruction_data.reg1_data, instruction_data.reg2_data, instruction_data.immediate, instruction_data.alu_control);
            int mem_read_data = Mem(instruction_data.reg2_data, alu_result);
            Write(instruction_data.write_register, alu_result, mem_read_data);
            if(control_signals.MemWrite){
                std::cout << "memory 0x" << std::hex << alu_result/sizeof(int) << " is modified to 0x" << std::hex << instruction_data.reg2_data << std::endl;
            }
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
    int Fetch(){
        int inst = imem[pc/sizeof(int)];//instruction memory is composed of ints so divide by sizeof(int)
        next_pc = pc + sizeof(int);
        return inst;
    }

    decoded_instruction Decode(int inst){//returns instruction_data type = r1_data,r2_data,write_reg,immediate,alu_ctrl (instr[12-14|30])
        Instruction instruction(inst);
        decoded_data data = instruction.decode();
        ControlUnit(data.opcode);
        decoded_instruction instruction_data = {rf[data.read_register_1], 
                                                rf[data.read_register_2], 
                                                data.write_register, 
                                                data.immediate, 
                                                data.alu_control};
        
        //Stall Detection
        
        //early branch detection
        //datapath mux version:
            int branch_target = pc + (instruction_data.immediate<<1);//calculate branch target adress, shift for instruction offset
            pc = (control_signals.Branch && (instruction_data.reg1_data==instruction_data.reg2_data)) ? branch_target : next_pc;//branch mux
        //sequential logic version:
        // if(control_signals.Branch && instruction_data.reg1_data==instruction_data.reg2_data){
        //     pc+=(instruction_data.immediate<<1);    //calculate branch target adress, shift for 2 byte instruction alignmnet
        //     //Flush if Incorrect Branch Prediction
        // }
        // else{pc = next_pc;}
        
        return instruction_data;
    }
    
    void ControlUnit(int opcode){
        //control_signal = {ALUSrc, MemToReg, RegWrite, MemRead, MemWrite, Branch, ALUOp}
        switch(opcode){
            case 0b0110011:
            case 0b0111011://R type
                control_signals = {0,0,1,0,0,0,0b10};
                break;
            case 0b0100011://save
                control_signals = {1,0,0,0,1,0,0b00};
                break;
            case 0b1100011://branch
                control_signals = {0,0,0,0,0,1,0b01};
                break;
            case 0b0000011://load
                control_signals = {1,1,1,1,0,0,0b00};
                break;
            case 0b010011://I type special i types not implemented (the ones that use funct7)
                control_signals = {1,0,1,0,0,0,0b11};
                break;
            //case jal,jalr not implemented
            default:
                std::cout << "Undefined Control Unit Behavior for opcode "<< std::bitset<7>(opcode) << std::endl;
        }
        return;
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
    int Execute(int read_reg1_data, int read_reg2_data, int immediate, int alu_control){
        int alu_result;
        int input2 = control_signals.ALUSrc ? immediate : read_reg2_data; //ALU_Src mux
        ALU_Op alu_operation = ALU_Control(control_signals.ALUOp, alu_control);
        switch(alu_operation){
            case ADD:
                alu_result = read_reg1_data + input2;
                break;
            case SUB:
                alu_result = read_reg1_data - input2;
                break;
            case AND:
                alu_result = read_reg1_data & input2;
                break;
            case OR:
                alu_result = read_reg1_data | input2;
                break;
            case nil:
                //do nothing (ie stall)
                break;
            default:
                std::cout << "Undefined ALU Behavior for alu_control "<< std::bitset<4>(alu_control) << std::endl;
        }
        return alu_result;
    }

    int Mem(int read_reg2_data, int alu_result){
        int address = alu_result/sizeof(int); //data memory is composed of ints so divide by sizeof(int)
        int mem_read_data;
        if(control_signals.MemRead){
            mem_read_data = dmem[address];
        }
        if(control_signals.MemWrite){
            dmem[address] = read_reg2_data;
        }
        return mem_read_data;
    }

    void Write(int write_reg, int alu_result, int mem_read_data){
        int write_data = control_signals.MemToReg ? mem_read_data : alu_result;     //write back mux
        total_clock_cycles+=1;
        std::cout << "\ntotal_clock_cycles: " << std::dec << total_clock_cycles << std::endl;
        if(control_signals.RegWrite){
            rf[write_reg] = write_data;
            std::cout << "x" << std::dec << write_reg << " is modified to 0x" << std::hex << write_data << std::endl;
        }
        return;
    }
};