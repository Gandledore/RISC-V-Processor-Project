#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <bitset>
#include <iomanip>
#include <deque>
#include <algorithm>
#include <utility>
#include <filesystem>

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
    bool jump=0;
    bool RegWrite=0;
    bool MemToReg=0;
    bool MemRead=0;
    bool MemWrite=0;
    int ALUOp=0;
    bool ALUSrc=0;
    bool Branch=0;
};
std::ostream& operator<<(std::ostream& os,ControlSignals& cs){                                                 //allows printing
    os << cs.ALUSrc << cs.MemToReg << cs.RegWrite << cs.MemRead << cs.MemWrite << cs.Branch << cs.ALUOp;
    return os;
}
bool operator==(ControlSignals& cs1, ControlSignals& cs2){                                                 //allows a node to be printed
    return cs1.jump==cs2.jump && cs1.RegWrite==cs2.RegWrite && cs1.MemToReg==cs2.MemToReg && cs1.MemRead==cs2.MemRead && cs1.MemWrite==cs2.MemWrite && cs1.ALUOp==cs2.ALUOp && cs1.ALUSrc==cs2.ALUSrc && cs1.Branch==cs2.Branch;
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
    int new_pc;
    bool flush_flag;
    bool stall_flag;
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

        //tracking variables
        program_loaded = false;
    }

    ~Processor(){//destructor
        delete[] dmem;
        delete[] imem;
    }
    
    void initialize_memory(std::string filename){
        //set up registers and memory to 0
        memset(rf, 0, sizeof(rf));
        memset(dmem, 0, dmem_size * sizeof(int));
        memset(imem, 0, imem_size * sizeof(int));
        
        if (filename == "sample_part1.txt") {//expected output: 0x2f at 0x70
            rf[1]=0x20; rf[2]=0x5;  rf[10]=0x70;    rf[11]=0x4;
            dmem[0x70/sizeof(int)]=0x5;   dmem[0x74/sizeof(int)]=0x10;
        }
        else if(filename=="sample_part2.txt"){//expected output: 0x3 at memory 0x20
            rf[8]=0x20; rf[10]=0x5; rf[11]=0x2; rf[12]=0xa; rf[13]=0xf;
        }
        else if(filename=="test1.txt"){//expected output: 0x14 at memory 0x5c 
            dmem[3]=1; dmem[4]=-3; dmem[5]=5; dmem[6]=-2; dmem[7]=19;
        }
        else if(filename=="test2.txt"){//expected output: 0x20 at memory 0x64
            dmem[5]=0xabc; dmem[6]=0xdef; dmem[7]=0x1234; dmem[8]=0x5678; dmem[9]=0x9abc;
        }
        else if(filename=="test3.txt"){//expected output 0x27a at memory 0x64
            dmem[3] = 0x34; dmem[4] = 0x77; dmem[5] = 0x28; dmem[6] = 0xb6; dmem[7] = 0xf1;
        }
        else if(filename=="data_dependency.txt"){//expected output 0xac at memory 0x30, 0x34, 0x38, 0x3c
            dmem[16] = 1, dmem[17] = 1; dmem[18] = 1; dmem[19] = 2;
        }
        else{
            std::cout << "Unknown Initial State. Using 0s." << std::endl;
        }
    }

    bool load(std::string filepath){//parse textfile, store to imem, return bool for success status
        std::ifstream file(filepath);
        if (!file) {
            std::cerr << "Error opening file!" << std::endl;
            return false; //failure
        }
        
        std::filesystem::path p(filepath);
        std::string filename = p.filename().string();
        initialize_memory(filename);

        std::cout << "Loading " << filename << " Instructions..." << std::endl;
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

    void run(){//public wrapper for private run_program function
        run_program();
    }

    private:
    void update_all_buffers(fetch_buffer* personal_fetch_buff, decode_buffer* personal_decode_buff, execute_buffer* personal_execute_buff, mem_buffer* personal_mem_buff){//update buffers with personal buffers
        if(!stall_flag && !flush_flag){//if no stall or flush is needed, update fetch buffers
            fetch_buff = *personal_fetch_buff;
        }
        else if(flush_flag){fetch_buff = {0,0};}
        decode_buff = *personal_decode_buff;
        execute_buff = *personal_execute_buff;
        mem_buff = *personal_mem_buff;
        stall_flag=false;
        flush_flag=false;
        pc = new_pc;
        return;
    }

    void run_program(){
        if(!program_loaded){
            std::cout << "No program loaded" << std::endl;
            return;
        }
        
        pc = 0;
        flush_flag = false;
        stall_flag = false;
        
        int inst;
        int max_instructions = 128;
        max_pc+=3*sizeof(int);
        total_clock_cycles = 0;
        while(pc<max_pc && pc>=0 && total_clock_cycles++<max_instructions){//within bounds of instruction memory
            std::cout << "\ntotal_clock_cycles: " << std::dec << total_clock_cycles << std::endl;

            fetch_buffer new_fetch = Fetch();
            decode_buffer new_decode = Decode();
            execute_buffer new_exe = Execute();
            mem_buffer new_mem = Mem();
            Write();

            update_all_buffers(&new_fetch, &new_decode, &new_exe, &new_mem);//update buffers with each stage's computations for this cycle

            std::cout << "pc is modified to 0x" << std::hex << pc << std::endl;
        }
        if(pc==max_pc){//program finished
            std::cout << "\nProgram Finished\n" << std::endl;
        }
        else{//Instruction out of bounds
            std::cout << "Instruction out of bounds" << std::endl;
            std::cout << "pc: " << std::hex << pc << std::endl;
            std::cout << "max_pc: " << std::hex << max_pc << std::endl;
            std::cout << std::dec << total_clock_cycles << "/" << max_instructions << " instructions executed" << std::endl;
        }
        return;
    }

    fetch_buffer Fetch(){
        std::cout << "Fetch Stage  | Fetching instruction " << (pc/sizeof(int)) << std::endl;

        int instruction = imem[pc/sizeof(int)];//instruction memory is composed of ints so divide by sizeof(int)
        fetch_buffer new_fetch = {instruction,pc};
        return new_fetch;
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
            case 0b0000011://load | rs1 needed in decode buffer no rs2
                return {2,5};
            case 0b1100111://jalr | rs1 neeeded in fetch buffer, no rs2
                return {1,5};
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
    int get_buffer_data(int buffer_index, int register_num=-1){//get data from the buffer based on the buffer index and register number (if needed)
        buffer_index = std::min(5,buffer_index);
        switch(buffer_index){
            case 3://get data from execute buffer (alu result) (eg R type)
                return execute_buff.alu_result;
            case 4://get data from mem buffer (mem read data) (eg load)
                return mem_buff.mem_read_data;
            case 5:
                return rf[register_num];//get data from write buffer (eg sw)
            default:
                throw std::runtime_error("Undefined Buffer Index Behavior for buffer index: "+std::to_string(buffer_index));
        }
    }
    
    decode_buffer Decode(){//returns instruction_data type = r1_data,r2_data,write_reg,immediate,alu_ctrl (instr[12-14|30])
        std::cout << "Decode Stage | ";
        decode_buffer new_decode_buffer;
        if(fetch_buff.instruction==0){
            new_decode_buffer.control_signals={0,0,0,0,0,0,0};
            dependencies.push_back({0,0});//push nop bubble into dependencies deque
            std::cout << "NOP Instruction" << std::endl;
            new_pc = pc+4;
            return new_decode_buffer;
        }
        Instruction instruction(fetch_buff.instruction);
        instruction.decode_instruction_fields();
        std::cout << " | ";
        decoded_data data = instruction.decode();
        
        ControlSignals control_signals = ControlUnit(data.opcode);
        if(data.write_register == 0){//if write register is 0, set to reg write signal to false to avoid writing to it
            control_signals.RegWrite = false;
        }
        
        //Stall Logic
        while(dependencies.size() > 3){//keep only most recent 3 instruction dependences (longest dependency is lw->branch (w/ early detection) = 3 instruction offset)
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

        std::cout << "Instruction Dependence Offsets: (" << instruction_dependence_offset_1 << ", " << instruction_dependence_offset_2 << ") | ";
        //Stall Calculation
        //buffer offset (relative to stage available) = stage needed - stage available + instruction offset
        //buffer index = stage needed + instruction offset
        //stall = buffer index < stage available
        auto [stage_needed1,stage_needed2] = calculate_stage_needed(data.opcode);
        int buffer_index1 = stage_needed1 + instruction_dependence_offset_1;
        int buffer_index2 = stage_needed2 + instruction_dependence_offset_2;
        stall_flag = (instruction_dependence_offset_1>0 && buffer_index1 < dependencies[dependence_index1].stage_available) 
                    || (instruction_dependence_offset_2>0 && buffer_index2 < dependencies[dependence_index2].stage_available);

        if(stall_flag){//if stall is needed, set control signals to 0, don't update pc so it fetches the same instruction, return
            std::cout << "Stalling" << std::endl;
            new_decode_buffer.control_signals = {0,0,0,0,0,0b00,0};//set control signals to 0
            dependencies.push_back({0,0});//push nop bubble into dependencies deque
            //return to avoid unecessary (and potentially incorrect) calculations: updating pc, flushing the pipeline, and updating the decode buffer
            new_pc = pc;
            return new_decode_buffer;
        }
        dependencies.push_back({data.write_register*control_signals.RegWrite, calculate_stage_available(data.opcode)});//push instruction update to dependencies deque
        new_decode_buffer.dependencies[0] = {buffer_index1, stage_needed1, instruction_dependence_offset_1>0 && stage_needed1<5};//rs1 dependency
        new_decode_buffer.dependencies[1] = {buffer_index2, stage_needed2, instruction_dependence_offset_2>0 && stage_needed2<5};//rs2 dependency
        
        std::cout << "Instruction Dependencies: " << new_decode_buffer.dependencies[0] << ", " << new_decode_buffer.dependencies[1] << std::endl;
        int reg1_data = (new_decode_buffer.dependencies[0].stage_needed==1 && new_decode_buffer.dependencies[0].is_dependent)? get_buffer_data(buffer_index1,data.read_register_1) : rf[data.read_register_1];
        int reg2_data = (new_decode_buffer.dependencies[1].stage_needed==1 && new_decode_buffer.dependencies[1].is_dependent)? get_buffer_data(buffer_index2,data.read_register_1) : rf[data.read_register_2];

        int next_pc = fetch_buff.pc + sizeof(int);

        //update decode buffer
        new_decode_buffer.next_pc = next_pc;
        new_decode_buffer.reg1 = data.read_register_1;
        new_decode_buffer.reg2 = data.read_register_2;
        new_decode_buffer.reg1_data = reg1_data;
        new_decode_buffer.reg2_data = reg2_data;
        new_decode_buffer.write_register = data.write_register;
        new_decode_buffer.immediate = data.immediate;
        new_decode_buffer.alu_control = data.alu_control;
        new_decode_buffer.control_signals = {control_signals.jump, 
                                        control_signals.RegWrite, 
                                        control_signals.MemToReg, 
                                        control_signals.MemRead, 
                                        control_signals.MemWrite, 
                                        control_signals.ALUOp, 
                                        control_signals.ALUSrc};
        
        //early branch detection
        int branch_target = fetch_buff.pc + data.immediate;//calculate branch target adress, instruction offset bit shift accounted for in decoding of immediate
        int jump_target = control_signals.Branch ? branch_target : reg1_data+data.immediate; //jump target mux
        bool jump_taken = (control_signals.jump || (control_signals.Branch && (reg1_data==reg2_data)));
        new_pc =  jump_taken ? jump_target : pc+4;//branch mux
        
        if(jump_taken){//if branch taken, flush
            flush_flag = true;
            std::cout << "Flushing" << std::endl;
        }
        return new_decode_buffer;
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
    
    execute_buffer Execute(){
        
        std::cout << "Execute Stage| ";
        execute_buffer new_execute_buffer;
        decode_buffer_control_signals nop_control_signals;
        if (decode_buff.control_signals==nop_control_signals){//if control signals are 0, do nothing (NOP), pass forward
            std::cout << "NOP Instruction" << std::endl; 
            new_execute_buffer.control_signals={0,0,0,0,0};
            return new_execute_buffer;
        }
        int alu_control = decode_buff.alu_control;
        int immediate = decode_buff.immediate;
        dependency rs1_dependency = decode_buff.dependencies[0];
        dependency rs2_dependency = decode_buff.dependencies[1];
        std::cout << rs1_dependency.buffer_index*rs1_dependency.is_dependent*(rs1_dependency.stage_needed==2) << ", " << rs2_dependency.buffer_index*rs2_dependency.is_dependent*(rs2_dependency.stage_needed==2) << " | ";
        int reg1_data = (rs1_dependency.stage_needed==2 && rs1_dependency.is_dependent)? get_buffer_data(rs1_dependency.buffer_index,decode_buff.reg1) : decode_buff.reg1_data;
        int reg2_data = (rs2_dependency.stage_needed==2 && rs2_dependency.is_dependent)? get_buffer_data(rs2_dependency.buffer_index,decode_buff.reg2) : decode_buff.reg2_data;
    
        int input2 = decode_buff.control_signals.ALUSrc ? immediate : reg2_data; //ALU_Src mux
        ALU_Op alu_operation = ALU_Control(decode_buff.control_signals.ALUOp, alu_control);
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
        new_execute_buffer.control_signals = {decode_buff.control_signals.jump, 
                                        decode_buff.control_signals.RegWrite, 
                                        decode_buff.control_signals.MemToReg, 
                                        decode_buff.control_signals.MemRead, 
                                        decode_buff.control_signals.MemWrite};
        new_execute_buffer.next_pc = decode_buff.next_pc;
        new_execute_buffer.alu_result = alu_result;
        new_execute_buffer.write_register = decode_buff.write_register;
        new_execute_buffer.reg2 = decode_buff.reg2;
        new_execute_buffer.reg2_data = reg2_data;
        new_execute_buffer.dependencies[0] = decode_buff.dependencies[0];
        new_execute_buffer.dependencies[1] = decode_buff.dependencies[1];
        return new_execute_buffer;
    }

    mem_buffer Mem(){
        std::cout << "Memory Stage | ";
        mem_buffer new_mem_buffer;
        execute_buffer_control_signals nop_control_signals;
        if (execute_buff.control_signals==nop_control_signals){//if control signals are 0, do nothing (NOP), pass forward
            std::cout << "NOP Instruction" << std::endl;
            new_mem_buffer.control_signals={0,0,0};
            return new_mem_buffer;
        }
        int alu_result = execute_buff.alu_result;
        dependency rs2_dependency = execute_buff.dependencies[1];
        int reg2_data = (rs2_dependency.stage_needed==3 && rs2_dependency.is_dependent)? get_buffer_data(rs2_dependency.buffer_index,execute_buff.reg2) : execute_buff.reg2_data;
        int next_pc = execute_buff.next_pc;
        int write_reg = execute_buff.write_register;

        int address = alu_result/sizeof(int); //data memory is composed of ints so divide by sizeof(int)
        int mem_read_data=execute_buff.control_signals.jump ? execute_buff.next_pc : alu_result; //jump mux
        if(execute_buff.control_signals.MemRead){
            mem_read_data = dmem[address];
            std::cout << "Read memory at 0x" << std::hex << alu_result << " = 0x" << std::hex << mem_read_data << std::endl;
        }
        if(execute_buff.control_signals.MemWrite){
            dmem[address] = reg2_data;
            std::cout << "memory 0x" << std::hex << alu_result << " is modified to 0x" << std::hex << reg2_data << std::endl;
        }
        if(!execute_buff.control_signals.MemRead && !execute_buff.control_signals.MemWrite){
            std::cout << "Did not use Memory" << std::endl;
        }

        //update mem buffer
        new_mem_buffer.control_signals = {execute_buff.control_signals.jump, 
                                    execute_buff.control_signals.RegWrite, 
                                    execute_buff.control_signals.MemToReg};
        new_mem_buffer.next_pc = next_pc;
        new_mem_buffer.alu_result = alu_result;
        new_mem_buffer.write_register = write_reg;
        new_mem_buffer.mem_read_data = mem_read_data;
        return new_mem_buffer;
    }

    void Write(){
        std::cout << "Write Stage  | ";
        mem_buffer_control_signals nop_control_signals;
        if (mem_buff.control_signals==nop_control_signals){//if control signals are 0, do nothing (NOP)
            std::cout << "NOP Instruction" << std::endl;
            return;
        }
        int mem_read_data = mem_buff.mem_read_data;
        int alu_result = mem_buff.alu_result;
        int next_pc = mem_buff.next_pc;
        int write_reg = mem_buff.write_register;

        int write_data = mem_buff.control_signals.MemToReg ? mem_read_data : alu_result;     //write back mux
        write_data = mem_buff.control_signals.jump ? next_pc : write_data; //jump mux
        if(mem_buff.control_signals.RegWrite){
            rf[write_reg] = write_data;
            std::cout << "x" << std::dec << write_reg << " is modified to 0x" << std::hex << write_data << std::endl;
        }
        else{std::cout << "Did not write" << std::endl;}
        return;
    }
};