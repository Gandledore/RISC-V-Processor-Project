#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <bitset>
#include <iomanip>
#include <deque>
#include <algorithm>
#include <utility>

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

struct instruction_update{//holds which instruction updates which register
    int register_destination=0;
    int stage_available=5;
};
std::ostream& operator<<(std::ostream& os,instruction_update& iu){
    os << "(rd: " << iu.register_destination << ", sa: " << iu.stage_available << ")";
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
    int write_buff;

    //deque for dependencies
    std::deque<instruction_update> dependencies;

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

    void fetch_all_buffers(fetch_buffer* personal_fetch_buff, decode_buffer* personal_decode_buff, execute_buffer* personal_execute_buff, mem_buffer* personal_mem_buff){//copy buffers to personal buffers
        *personal_fetch_buff = fetch_buff;
        *personal_decode_buff = decode_buff;
        *personal_execute_buff = execute_buff;
        *personal_mem_buff = mem_buff;
        return;
    }

    void run(){//run loaded program
        if(!program_loaded){
            std::cout << "No program loaded" << std::endl;
            return;
        }
        int inst;
        int instruction_count = 0;
        int max_instructions = 100;
        max_pc+=3*4;
        fetch_buffer personal_fetch_buff;//copy fetch buffer to personal fetch buffer
        execute_buffer personal_execute_buff;//copy execute buffer to personal execute buffer
        decode_buffer personal_decode_buff;//copy decode buffer to personal decode buffer
        mem_buffer personal_mem_buff;//copy mem buffer to personal mem buffer
        while(pc<max_pc && pc>=0 && instruction_count++<max_instructions){//within bounds of instruction memory
            Fetch();
            fetch_all_buffers(&personal_fetch_buff, &personal_decode_buff, &personal_execute_buff, &personal_mem_buff);//copy buffers to personal buffers
            Decode(personal_fetch_buff);
            Execute(personal_decode_buff);
            Mem(personal_execute_buff);
            Write(personal_mem_buff);
            std::cout << "pc is modified to 0x" << std::hex << pc << std::endl;
        }
        if(pc==max_pc){
            std::cout << "\nProgram Finished\n" << std::endl;
        }
        else{
            std::cout << "Instruction out of bounds" << std::endl;
            std::cout << "pc: " << std::hex << pc << std::endl;
            std::cout << "max_pc: " << std::hex << max_pc << std::endl;
            std::cout << std::dec << instruction_count << "/" << max_instructions << " instructions executed" << std::endl;
        }
        return;
    }

    private:
    void Fetch(){
        std::cout << "\ntotal_clock_cycles: " << std::dec << total_clock_cycles << std::endl;
        fetch_buff.instruction = imem[pc/sizeof(int)];//instruction memory is composed of ints so divide by sizeof(int)
        fetch_buff.next_pc = pc + sizeof(int);
        std::cout << "Fetch Stage  | Fetched instruction " << (pc/sizeof(int)) << std::endl;
        total_clock_cycles+=1;
    }

    //indexes:
    //0   1   2   3   4   5
    //  F   D   E   M   W
    int calculate_stage_available(int opcode){
        switch(opcode){//stage available = stage needed - instruction offset
            case 0b0110011://R 
            case 0b0111011://R type | available in execute buffer
            case 0b1100111://jalr | available in execute buffer
            case 0b0010011://I type | available in execute buffer
                return 3;
            case 0b0000011://load | available in mem buffer
                return 4;
            case 0b1101111://jal | available in fetch buffer
                return 1;
            case 0b0100011://sw | doesn't cause dependencies
            case 0b1100011://branch | doesn't cause dependencies
            case 0b0000000://nop
                return -1;
            default:
                throw std::runtime_error("Undefined Stage Available Behavior for opcode "+std::bitset<7>(opcode).to_string());
        }
    }
    std::pair<int,int> calculate_stage_needed(int opcode){
        switch(opcode){//stage needed = stage available + instruction offset
            case 0b0110011://R 
            case 0b0111011://R type | rs1,2 needed in decode buffer
                return {2,2};
            case 0b0010011://I type | rs1 needed in decode buffer, no rs2
            case 0b1100111://jalr | rs1 neeeded in decode buffer, no rs2
            case 0b0000011://load | rs1 needed in decode buffer no rs2
                return {2,5};
            case 0b0100011://sw   | rs1 needed in decode buffer rs2 needed in execute buffer
                return {2,3};
            case 0b1100011://branch | needed in fetch_buffer (actually just needs correct info by the end of fetch stage)
                return {1,1};
            case 0b1101111://jal | no rs1, rs2, needs pc+4 in mem buffer (which is guaranteed)
            case 0b0000000://nop
                return {5,5};
            default:
                throw std::runtime_error("Undefined Stage Needed Behavior for opcode "+std::bitset<7>(opcode).to_string());
        }
    }
    int get_buffer_data(int buffer_index){
        switch(buffer_index){
            case 3://get data from execute buffer (alu result) (eg R type)
                return execute_buff.alu_result;
            case 4://get data from mem buffer (mem read data) (eg load)
                return mem_buff.mem_read_data;
            case 5:
                return write_buff;//get data from write buffer (eg sw)
            default:
                throw std::runtime_error("Undefined Buffer Index Behavior for buffer index: "+std::to_string(buffer_index));
        }
    }
    
    void Decode(fetch_buffer personal_fetch_buff){//returns instruction_data type = r1_data,r2_data,write_reg,immediate,alu_ctrl (instr[12-14|30])
        std::cout << "Decode Stage | ";
        if (flush_flag){//if flush is true, set decode buffer to flush
            std::cout << "Flushing" << std::endl;
            decode_buff.control_signals = {0,0,0,0,0,0b00,0};
            flush_flag = false;
            return;
        }

        Instruction instruction(personal_fetch_buff.instruction);
        instruction.decode_instruction_fields();
        decoded_data data = instruction.decode();
        
        ControlSignals control_signals = ControlUnit(data.opcode);
        if(data.write_register == 0){//if write register is 0, set to reg write signal to false to avoid writing to it
            control_signals.RegWrite = false;
        }
        
        //Stall Logic
        if(dependencies.size() > 3){//keep only most recent 3 instruction dependences (longest dependency is lw->branch (w/ early detection) = 3 instruction offset)
            dependencies.pop_front();
        }

        //check if there are any dependencies in the pipeline (ie if the instruction is using a register that is being written to by another instruction)
        auto dependence1 = std::find_if(dependencies.rbegin(), dependencies.rend(), [&](instruction_update& previous_instruction){
            return previous_instruction.register_destination == data.read_register_1 && previous_instruction.register_destination != 0;
        });
        auto dependence2 = std::find_if(dependencies.rbegin(), dependencies.rend(), [&](instruction_update& previous_instruction){
            return previous_instruction.register_destination == data.read_register_2 && previous_instruction.register_destination != 0;
        });
        int dependence_index1 = (dependence1==dependencies.rend())? dependencies.size() : std::distance(dependencies.begin(), std::prev(dependence1.base()));
        int dependence_index2 = (dependence2==dependencies.rend())? dependencies.size() : std::distance(dependencies.begin(), std::prev(dependence2.base()));
        int instruction_dependence_offset_1 = dependencies.size() - dependence_index1;
        int instruction_dependence_offset_2 = dependencies.size() - dependence_index2;

        // std::cout << "\nDependence Index 1: " << dependence_index1 << " | Dependence Index 2: " << dependence_index2 << std::endl;
        std::cout << "Instruction Dependence Offsets: (" << instruction_dependence_offset_1 << ", " << instruction_dependence_offset_2 << ") | ";
        //Stall Calculation
        //buffer offset (relative to stage available) = stage needed - stage available + instruction offset
        //buffer index = stage needed + instruction offset
        //stall = buffer index < stage available
        auto [stage_needed1,stage_needed2] = calculate_stage_needed(data.opcode);
        int buffer_index1 = stage_needed1 + instruction_dependence_offset_1;
        int buffer_index2 = stage_needed2 + instruction_dependence_offset_2;
        bool stall = (instruction_dependence_offset_1>0 && buffer_index1 < dependencies[dependence_index1].stage_available) 
                        || (instruction_dependence_offset_1>0 && buffer_index2 < dependencies[dependence_index2].stage_available);

        if(stall){//if stall is needed, set control signals to 0, don't update pc so it fetches the same instruction, return
            std::cout << "Stalling" << std::endl;
            decode_buff.control_signals = {0,0,0,0,0,0b00,0};//set control signals to 0
            dependencies.push_back({0,0});//push nop bubble into dependencies deque
            //return to avoid unecessary (and potentially incorrect) calculations: updating pc, flushing the pipeline, and updating the decode buffer
                    //print dependencies deque for debugging
            return;
        }
        dependencies.push_back({data.write_register*control_signals.RegWrite, calculate_stage_available(data.opcode)});//push instruction update to dependencies deque
        decode_buff.dependencies[0] = {buffer_index1, stage_needed1, instruction_dependence_offset_1>0 && stage_needed1<5 && buffer_index1<6};//rs1 dependency
        decode_buff.dependencies[1] = {buffer_index2, stage_needed2, instruction_dependence_offset_2>0 && stage_needed2<5 && buffer_index2<6};//rs2 dependency
        
        std::cout << "Instruction Dependencies: " << decode_buff.dependencies[0] << ", " << decode_buff.dependencies[1] << std::endl;
        int reg1_data = (decode_buff.dependencies[0].stage_needed==1 && decode_buff.dependencies[0].is_dependent)? get_buffer_data(buffer_index1) : rf[data.read_register_1];
        int reg2_data = (decode_buff.dependencies[1].stage_needed==1 && decode_buff.dependencies[1].is_dependent)? get_buffer_data(buffer_index2) : rf[data.read_register_2];

        //update decode buffer
        decode_buff.next_pc = personal_fetch_buff.next_pc;
        decode_buff.reg1_data = reg1_data;
        decode_buff.reg2_data = reg2_data;
        decode_buff.write_register = data.write_register;
        decode_buff.immediate = data.immediate;
        decode_buff.alu_control = data.alu_control;
        decode_buff.control_signals = {control_signals.jump, 
                                        control_signals.RegWrite, 
                                        control_signals.MemToReg, 
                                        control_signals.MemRead, 
                                        control_signals.MemWrite, 
                                        control_signals.ALUOp, 
                                        control_signals.ALUSrc};
        
        //early branch detection
        int branch_target = pc + data.immediate;//calculate branch target adress, instruction offset bit shift accounted for in decoding of immediate
        int jump_target = control_signals.Branch ? branch_target : reg1_data+data.immediate; //jump target mux
        pc = (control_signals.jump || (control_signals.Branch && (reg1_data==reg2_data))) ? jump_target : personal_fetch_buff.next_pc;//branch mux
        
        if(pc != personal_fetch_buff.next_pc){//if branch not taken, control signals are valid
            flush_flag = true;
            std::cout << "Marked for flush" << std::endl;
        }
        std::cout << "\t\tPotential Dependencies: ";
        for(auto& dep : dependencies){
            std::cout << dep << " | ";
        }
        std::cout << std::endl;
        return;
    }
    ControlSignals ControlUnit(int opcode){
        //control_signal = {Jump, RegWrite, MemToReg, MemRead, MemWrite, ALUOp, ALUSrc, Branch}
        switch(opcode){
            case 0b0110011:
            case 0b0111011://R type
                return {0,1,0,0,0,0b10,0,0};
            case 0b0100011://save
                return {0,0,0,0,1,0b00,1,0};
            case 0b1100011://branch
                return {0,0,0,0,0,0b01,0,1};
            case 0b0000011://load
                return {0,1,1,1,0,0b00,1,0};
            case 0b0010011://I type special i types not implemented (the ones that use funct7)
                return {0,1,0,0,0,0b11,1,0};
            case 0b1101111://jal
                return {1,1,0,0,0,0b00,0,1};
            case 0b1100111://jalr
                return {1,1,0,0,0,0b00,0,0};
            case 0b0000000://nop
                return {0,0,0,0,0,0b00,0,0};
            default:
                throw std::runtime_error("Undefined Control Unit Behavior for opcode "+std::bitset<7>(opcode).to_string());
        }
    }
    
    enum ALU_Op{nil, ADD, SUB, AND, OR};
    ALU_Op ALU_Control(int alu_op, int alu_ctrl){
        //alu_control = instruction[12-14|30]
        switch(alu_op){
            case 0b00://lw,sw
                return ADD;
            case 0b01://beq
                return SUB;
            case 0b11://I type
                alu_ctrl &= 0b1110;//rightmost bit part of immediate, first 3 are funct3
                //no break, fallthrough to R
            case 0b10://R type
                switch(alu_ctrl){
                    case 0b0000:
                        return ADD;
                    case 0b0001:
                        return SUB;
                    case 0b1110:
                        return AND;
                    case 0b1100:
                        return OR;
                    default:
                        throw std::runtime_error("Undefined ALU Control Behavior for alu_ctrl "+std::bitset<4>(alu_ctrl).to_string());
                }
                break;
            default:
                throw std::runtime_error("Undefined ALU Control Behavior for alu_op "+std::bitset<2>(alu_op).to_string());
        }
    }
    
    void Execute(decode_buffer personal_decode_buffer){
        std::cout << "Execute Stage| ";
        decode_buffer_control_signals nop_control_signals;
        if (personal_decode_buffer.control_signals==nop_control_signals){std::cout << "NOP Instruction" << std::endl; execute_buff.control_signals={0,0,0,0,0};return;}//if control signals are 0, do nothing (NOP)
        int alu_control = personal_decode_buffer.alu_control;
        int immediate = personal_decode_buffer.immediate;
        dependency rs1_dependency = personal_decode_buffer.dependencies[0];
        dependency rs2_dependency = personal_decode_buffer.dependencies[1];
        std::cout << rs1_dependency.buffer_index << ", " << rs2_dependency.buffer_index << " | ";
        int reg1_data = (rs1_dependency.stage_needed==2 && rs1_dependency.is_dependent)? get_buffer_data(rs1_dependency.buffer_index) : personal_decode_buffer.reg1_data;
        int reg2_data = (rs2_dependency.stage_needed==2 && rs2_dependency.is_dependent)? get_buffer_data(rs2_dependency.buffer_index) : personal_decode_buffer.reg2_data;
        
        int input2 = personal_decode_buffer.control_signals.ALUSrc ? immediate : reg2_data; //ALU_Src mux
        ALU_Op alu_operation = ALU_Control(personal_decode_buffer.control_signals.ALUOp, alu_control);
        int alu_result;
        switch(alu_operation){
            case ADD://also for nop
                alu_result = reg1_data + input2;
                std::cout << std::dec << alu_result << " = " << reg1_data << " + " << input2 << std::endl;
                break;
            case SUB:
                alu_result = reg1_data - input2;
                std::cout << std::dec << alu_result << " = " << reg1_data << " - " << input2 << std::endl;
                break;
            case AND:
                alu_result = reg1_data & input2;
                std::cout << std::dec << alu_result << " = " << reg1_data << " & " << input2 << std::endl;
                break;
            case OR:
                alu_result = reg1_data | input2;
                std::cout << std::dec << alu_result << " = " << reg1_data << " | " << input2 << std::endl;
                break;
            default:
                throw std::runtime_error("Undefined ALU Behavior for alu_control "+std::bitset<4>(alu_control).to_string());
        }

        //update execute buffer
        execute_buff.control_signals = {personal_decode_buffer.control_signals.jump, 
                                        personal_decode_buffer.control_signals.RegWrite, 
                                        personal_decode_buffer.control_signals.MemToReg, 
                                        personal_decode_buffer.control_signals.MemRead, 
                                        personal_decode_buffer.control_signals.MemWrite};
        execute_buff.next_pc = personal_decode_buffer.next_pc;
        execute_buff.alu_result = alu_result;
        execute_buff.write_register = personal_decode_buffer.write_register;
        execute_buff.reg2_data = reg2_data;
        execute_buff.dependencies[0] = personal_decode_buffer.dependencies[0];
        execute_buff.dependencies[1] = personal_decode_buffer.dependencies[1];
        return;
    }

    void Mem(execute_buffer personal_execute_buffer){
        std::cout << "Memory Stage | ";
        execute_buffer_control_signals nop_control_signals;
        if (personal_execute_buffer.control_signals==nop_control_signals){std::cout << "NOP Instruction" << std::endl; mem_buff.control_signals={0,0,0};return;}//if control signals are 0, do nothing (NOP)
        int alu_result = personal_execute_buffer.alu_result;
        dependency rs2_dependency = personal_execute_buffer.dependencies[1];
        int reg2_data = (rs2_dependency.stage_needed==3 && rs2_dependency.is_dependent)? get_buffer_data(rs2_dependency.buffer_index) : personal_execute_buffer.reg2_data;
        int next_pc = personal_execute_buffer.next_pc;
        int write_reg = personal_execute_buffer.write_register;

        int address = alu_result/sizeof(int); //data memory is composed of ints so divide by sizeof(int)
        int mem_read_data=alu_result;
        if(personal_execute_buffer.control_signals.MemRead){
            mem_read_data = dmem[address];
            std::cout << "Read memory at 0x" << std::hex << alu_result << " = 0x" << std::hex << mem_read_data << std::endl;
        }
        if(personal_execute_buffer.control_signals.MemWrite){
            dmem[address] = reg2_data;
            std::cout << "memory 0x" << std::hex << alu_result << " is modified to 0x" << std::hex << reg2_data << std::endl;
        }
        if(!personal_execute_buffer.control_signals.MemRead && !personal_execute_buffer.control_signals.MemWrite){
            std::cout << "Did not use Memory" << std::endl;
        }

        //update mem buffer
        mem_buff.control_signals = {personal_execute_buffer.control_signals.jump, 
                                    personal_execute_buffer.control_signals.RegWrite, 
                                    personal_execute_buffer.control_signals.MemToReg};
        mem_buff.next_pc = next_pc;
        mem_buff.alu_result = alu_result;
        mem_buff.write_register = write_reg;
        mem_buff.mem_read_data = mem_read_data;
        return;
    }

    void Write(mem_buffer personal_mem_buff){
        std::cout << "Write Stage  | ";
        mem_buffer_control_signals nop_control_signals;
        if (personal_mem_buff.control_signals==nop_control_signals){std::cout << "NOP Instruction" << std::endl;return;}//if control signals are 0, do nothing (NOP)
        int mem_read_data = personal_mem_buff.mem_read_data;
        int alu_result = personal_mem_buff.alu_result;
        int next_pc = personal_mem_buff.next_pc;
        int write_reg = personal_mem_buff.write_register;

        int write_data = personal_mem_buff.control_signals.MemToReg ? mem_read_data : alu_result;     //write back mux
        write_data = personal_mem_buff.control_signals.jump ? next_pc : write_data; //jump mux
        if(personal_mem_buff.control_signals.RegWrite){
            rf[write_reg] = write_data;
            write_buff = write_data;
            std::cout << "x" << std::dec << write_reg << " is modified to 0x" << std::hex << write_data << std::endl;
        }
        else{std::cout << "Did not write" << std::endl;}
        return;
    }
};