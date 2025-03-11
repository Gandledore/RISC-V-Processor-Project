#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include "instruction.h"
#include <bitset>

int binary_to_dec(std::string num){
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
std::ostream& operator<<(std::ostream& os,ControlSignals& cs){                                                 //allows a node to be printed
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
std::ostream& operator<<(std::ostream& os,decoded_instruction& di){                                                 //allows a node to be printed
    os << "r1: " << di.reg1_data << " | r2: " << di.reg2_data << " | wr: " << di.write_register << " | imm: " << di.immediate << " | alu: " << std::bitset<4>(di.alu_control);
    return os;
}

class Processor{
    //program counter
    int pc;
    ControlSignals control_signals;
    //data
    int rf[32];
    int* dmem;
    int* imem;
    int imem_size;
    int dmem_size;

    public:
    Processor(int imem_size = 32, int dmem_size=32){//parse textfile, store to imem
        this->imem_size = imem_size;
        this->dmem_size = dmem_size;
        dmem = new int[imem_size];
        imem = new int[dmem_size];

        memset(rf, 0, sizeof(rf));
        memset(dmem, 0, dmem_size * sizeof(int));
        memset(imem, 0, imem_size * sizeof(int));

        //sample_1 initial state
        rf[1]=0x20; rf[2]=0x5;  rf[10]=0x70;    rf[11]=0x4;
        dmem[0x70/4]=0x5;   dmem[0x74/4]=0x10;

        pc = 0;
        control_signals = {0,0,0,0,0,0,0};
    }
    
    bool load(std::string filepath){
        std::ifstream file(filepath);
        if (!file) {
            std::cerr << "Error opening file!" << std::endl;
            return false; // Exit with an error code
        }
        std::string line;
        for(int i=0;i<32;i++){
            if(!std::getline(file, line)){
                break;
            }
            imem[i] = binary_to_dec(line.substr(0,line.size()-1));//skips the newline character at thend
        }
        file.close(); // Close the file
        return true;
    }
    
    void run(){
        int inst;
        decoded_instruction instruction_data;
        while(true){
            inst = Fetch();
            if(inst==0){break;}//instruction==0 implies finished file, since imem 0 except for actual instructions
            instruction_data = Decode(inst);
        }
    }
    
    int Fetch(){
        int inst = imem[pc/4];
        pc+=4;
        return inst;
    }

    decoded_instruction Decode(int inst){//returns instruction_data type = r1,r2,w1,imm,alu_ctrl
        Instruction instruction(inst);
        decoded_data data = instruction.decode();
        ControlUnit(data.opcode);
        decoded_instruction instruction_data = {rf[data.read_register_1], 
                                                rf[data.read_register_2], 
                                                data.write_register, 
                                                data.immediate, 
                                                data.alu_control};
        
        //print debugging info to test
        instruction.decode_instruction_fields();
        std::cout << "Control Signals: " << control_signals << std::endl;
        std::cout << "Instruction Data | " << instruction_data << "\n" << std::endl;

        return instruction_data;
    }
    void ControlUnit(int opcode){
        //control_signal = {ALUSrc, MemToReg, RegWrite, MemRead, MemWrite, Branch, ALUOp}
        switch(opcode){
            case 0b0110011:
            case 0b0111011://R type
                control_signals = {0,0,1,0,0,0,2};
                break;
            case 0b0100011://save
                control_signals = {1,0,0,0,1,0,0};
                break;
            case 0b1100011://branch
                control_signals = {0,0,0,0,0,1,1};
                break;
            case 0b0000011://load
                control_signals = {1,1,1,1,0,0,0};
                break;
            case 0b010011://I type special i types not implemented
                control_signals = {1,0,1,0,0,0,3};
                break;
            //cases jal,jalr not implemented
            default:
                std::cout << "Undefined ControlUnit Behavior for opcode "<< std::bitset<7>(opcode) << std::endl;
        }
        return;
    }
    int Mem(int read_reg2_data, int alu_result){
        int address = alu_result/4;
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
        int write_data = control_signals.MemToReg ? mem_read_data : alu_result;
        if(control_signals.RegWrite){
            rf[write_reg] = write_data;
        }
        return;
    }
    ~Processor(){
        delete[] dmem;
        delete[] imem;
    }
    
};