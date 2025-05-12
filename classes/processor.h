#ifndef PROCESSOR_H
#define PROCESSOR_H

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

#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <barrier>
#include <functional>

#include "instruction.h"
#include "buffers/buffers.h"

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
bool operator==(const ControlSignals& cs1, const ControlSignals& cs2){                                                 //allows a node to be printed
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
    std::mutex rf_mtx;
    int* dmem;
    std::mutex dmem_mtx;
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

    bool program_loaded;
    int max_pc;//last instruction address + sizeof(int)
    bool program_finished;
    std::barrier<> clock_tick;
    int total_clock_cycles = 0;
    int max_clock_cycles = 128;
    std::mutex print_mtx;

    public:
    Processor(int imem_size = 32, int dmem_size=32)
        :clock_tick(6)//5 stage threads + 1 clock thread
        {//set up instruction memory, data memory, and registers, and default tracking vars (pc, control signals, total clock cycles, program_loaded)
        this->imem_size = imem_size;
        this->dmem_size = dmem_size;
        dmem = new int[dmem_size];
        imem = new int[imem_size];

        //tracking variables
        program_loaded = false;
        program_finished = false;
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

    void run_program(){
        if(!program_loaded){
            std::cout << "No program loaded" << std::endl;
            return;
        }
        
        pc = 0;
        flush_flag = false;
        stall_flag = false;
        
        std::jthread clock_thread(std::bind(&Processor::clock, this));
        clock_thread.join();

        if(pc==max_pc){//program finished
            std::cout << "\nProgram Finished\n" << std::endl;
        }
        else{//Instruction out of bounds
            std::cout << "Instruction out of bounds" << std::endl;
            std::cout << "pc: " << std::hex << pc << std::endl;
            std::cout << "max_pc: " << std::hex << max_pc << std::endl;
            std::cout << std::dec << total_clock_cycles << "/" << max_clock_cycles << " instructions executed" << std::endl;
        }
    }
    void clock(){
        //spawn threads
        std::stop_source program_done;
        std::stop_token stop_token = program_done.get_token();

        std::vector<std::jthread> stages;
        stages.emplace_back(std::bind(&Processor::Fetch, this, stop_token));
        stages.emplace_back(std::bind(&Processor::Decode, this, stop_token));
        stages.emplace_back(std::bind(&Processor::Execute, this, stop_token));
        stages.emplace_back(std::bind(&Processor::Mem, this, stop_token));
        stages.emplace_back(std::bind(&Processor::Write, this, stop_token));

        int endgame=4;
        std::ostringstream clock_stream;
        while(total_clock_cycles++<max_clock_cycles && pc<=max_pc && endgame>0){
            clock_stream.str("");
            clock_stream << "\ntotal clock cycles: " << std::dec << total_clock_cycles;
            debug_msg(clock_stream);

            clock_tick.arrive_and_wait();//wait to start all stages for synchronization (rising edge)
            //cycle in progress
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Slow down for humans
            
            //cycle done, begin cycle cleanup
            clock_tick.arrive_and_wait();//falling edge
            if(!program_finished){ pc = new_pc;}//keep pc pointing to max_pc if done so no out of bounds
            else{endgame--;pc=max_pc;}//finish program in 4 more cycles (pc should be max pc natrually)
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Slow down for humans
            clock_stream.str("");
            clock_stream << "pc is modified to 0x" << std::hex << pc;
            debug_msg(clock_stream);
        }
        program_done.request_stop();
        clock_tick.arrive_and_drop();
        return;
    }
    void Fetch(std::stop_token stop_token){
        std::unique_lock<std::mutex> fetch_lock(fetch_buff.get_mutex(),std::defer_lock);
        std::ostringstream fetch_stream;
        while(true){
            clock_tick.arrive_and_wait();//wait to start all threads in sync (rising edge)
            if(stop_token.stop_requested()){break;}
            fetch_stream.str("");
            fetch_stream << "Fetch Stage  | Fetching " << ((pc==max_pc)? "NOP Instruction (max_pc)" : ("instruction " + std::to_string(pc/sizeof(int))));

            int instruction = imem[pc/sizeof(int)];//instruction memory is composed of ints so divide by sizeof(int)
            int inst_pc = pc;
            clock_tick.arrive_and_wait();//falling edge to write

            fetch_buffer_data new_fetch_buffer;
            if(!stall_flag && !flush_flag){//if no stall or flush is needed, update fetch buffers
                new_fetch_buffer = {instruction,inst_pc};
            }
            else if(flush_flag){//if flush, ignore currently fetched instruction and place nop
                new_fetch_buffer = {0,0};
            }
            
            if(!stall_flag){
                fetch_lock.lock();
                fetch_buff.data = new_fetch_buffer;
                fetch_lock.unlock();
            }
            
            stall_flag=false;
            flush_flag=false;
            debug_msg(fetch_stream);
        }
        clock_tick.arrive_and_drop();
        return;
    }

    //indexes:
    //0   1   2   3   4   5
    //  F   D   E   M   W
    int calculate_stage_available(int opcode){
        switch(opcode){//stage available = stage needed - instruction offset
            case 0b0110011://R 
            case 0b0111011://R type | available in execute buffer
            case 0b0010011://I type | available in execute buffer
            return 3;
            case 0b0000011://load | available in mem buffer
            return 4;
            case 0b1100111://jalr | available in execute buffer
            case 0b1101111://jal | available in fetch buffer
                return 2;
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
        forwarding_buffer* target_buffer;
        switch(buffer_index){
            case 2:
                target_buffer = &decode_buff;
                break;
                // return decode_buff.data.next_pc;//get data from decode buffer (eg rs1, rs2)
            case 3://get data from execute buffer (alu result) (eg R type)
                target_buffer = &execute_buff;
                break;
                // return execute_buff.data.alu_result;
            case 4://get data from mem buffer (mem read data) (eg load)
                target_buffer = &mem_buff;
                break;
                // return mem_buff.data.mem_read_data;
            case 5:
                target_buffer = nullptr;
                break;
                // return rf[register_num];//get data from write buffer (eg sw)
            default:
                throw std::runtime_error("Undefined Buffer Index Behavior for buffer index: "+std::to_string(buffer_index));
        }
        int answer = 0;
        if(target_buffer==nullptr){
            answer = reg_file_access(true,register_num);
        }
        else{
            std::unique_lock<std::mutex> lock(target_buffer->get_mutex());
            answer = target_buffer->get_data();
            lock.unlock();
        }
        return answer;
    }
    
    int reg_file_access(bool read,int reg_target, int write_data=0){
        int reg_data=0;
        std::unique_lock<std::mutex> rf_lock(rf_mtx);
        if(read){
            reg_data = rf[reg_target];
        }
        else{
            rf[reg_target] = write_data;
        }
        rf_lock.unlock();
        return reg_data;
    }
    int dmem_access(bool read, int address, int write_data=0){
        int mem_data=0;
        std::unique_lock<std::mutex> dmem_lock(dmem_mtx);
        if(read){
            mem_data = dmem[address];
        }
        else{
            dmem[address] = write_data;
        }
        dmem_lock.unlock();
        return mem_data;
    }

    void Decode(std::stop_token stop_token){//returns instruction_data type = r1_data,r2_data,write_reg,immediate,alu_ctrl (instr[12-14|30])
        std::unique_lock<std::mutex> fetch_lock(fetch_buff.get_mutex(),std::defer_lock);
        std::unique_lock<std::mutex> decode_lock(decode_buff.get_mutex(),std::defer_lock);
        std::ostringstream decode_stream;
        while(true){
            clock_tick.arrive_and_wait();//wait to start all threads in sync (rising edge)
            if(stop_token.stop_requested()){break;}
            
            //read fetch buffer
            fetch_lock.lock();
            fetch_buffer_data fetchBufferData = fetch_buff.data;
            fetch_lock.unlock();
            
            decode_stream.str("");
            decode_stream << "Decode Stage | ";
            decode_buffer_data new_decode_buffer;
            if(fetchBufferData.instruction==0){
                new_decode_buffer.control_signals={0,0,0,0,0,0,0};
                dependencies.push_back({0,0});//push nop bubble into dependencies deque
                decode_stream << "NOP Instruction";
                new_pc = pc+4;
                clock_tick.arrive_and_wait();//falling edge

                decode_lock.lock();
                decode_buff.data = new_decode_buffer;
                decode_lock.unlock();

                debug_msg(decode_stream);
                continue;
            }
            Instruction instruction(fetchBufferData.instruction);
            decode_stream << instruction.decode_instruction_fields().str() << " | ";
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
    
            decode_stream << "Instruction Dependence Offsets: (" << instruction_dependence_offset_1 << ", " << instruction_dependence_offset_2 << ") | ";
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
                decode_stream << "Stalling";
                new_decode_buffer.control_signals = {0,0,0,0,0,0b00,0};//set control signals to 0
                dependencies.push_back({0,0});//push nop bubble into dependencies deque
                //return to avoid unecessary (and potentially incorrect) calculations: updating pc, flushing the pipeline, and updating the decode buffer
                new_pc = pc;
                clock_tick.arrive_and_wait();//falling edge

                decode_lock.lock();
                decode_buff.data = new_decode_buffer;
                decode_lock.unlock();

                debug_msg(decode_stream);
                continue;
            }
            dependencies.push_back({data.write_register*control_signals.RegWrite, calculate_stage_available(data.opcode)});//push instruction update to dependencies deque
            new_decode_buffer.dependencies[0] = {buffer_index1, stage_needed1, instruction_dependence_offset_1>0 && stage_needed1<5};//rs1 dependency
            new_decode_buffer.dependencies[1] = {buffer_index2, stage_needed2, instruction_dependence_offset_2>0 && stage_needed2<5};//rs2 dependency
            
            decode_stream << "Instruction Dependencies: " << new_decode_buffer.dependencies[0] << ", " << new_decode_buffer.dependencies[1];
            int reg1_data = (new_decode_buffer.dependencies[0].stage_needed==1 && new_decode_buffer.dependencies[0].is_dependent)? get_buffer_data(buffer_index1,data.read_register_1) : reg_file_access(true,data.read_register_1);
            int reg2_data = (new_decode_buffer.dependencies[1].stage_needed==1 && new_decode_buffer.dependencies[1].is_dependent)? get_buffer_data(buffer_index2,data.read_register_1) : reg_file_access(true,data.read_register_2);
            
            //early branch detection
            int branch_target = fetchBufferData.pc + data.immediate;//calculate branch target adress, instruction offset bit shift accounted for in decoding of immediate
            int jump_target = control_signals.Branch ? branch_target : reg1_data+data.immediate; //jump target mux
            bool jump_taken = (control_signals.jump || (control_signals.Branch && (reg1_data==reg2_data)));
            new_pc =  jump_taken ? jump_target : pc+4;//branch mux
            
            if(jump_taken){//if branch taken, flush
                flush_flag = true;
                decode_stream << "\nFlushing";
            }
            
            //program is finished if pc is pointing after all instructions 
            //  and we aren't jumping back to earlier in the program
            program_finished = ((pc==max_pc && !flush_flag) || (new_pc==max_pc && flush_flag)) || program_finished;

            clock_tick.arrive_and_wait(); // falling edge (done reading data from buffers)

            int next_pc = fetchBufferData.pc + sizeof(int);
    
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
            
            decode_lock.lock();
            decode_buff.data = new_decode_buffer;
            decode_lock.unlock();
            
            debug_msg(decode_stream);
        }
        clock_tick.arrive_and_drop();
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
    
    void Execute(std::stop_token stop_token){
        std::unique_lock<std::mutex> decode_lock(decode_buff.get_mutex(),std::defer_lock);
        std::unique_lock<std::mutex> execute_lock(execute_buff.get_mutex(),std::defer_lock);
        std::ostringstream execute_stream;
        while(true){
            clock_tick.arrive_and_wait();//wait to start all threads in sync (rising edge)
            if(stop_token.stop_requested()){break;}
            
            decode_lock.lock();
            decode_buffer_data decodeBufferData = decode_buff.data;
            decode_lock.unlock();

            execute_stream.str("");
            execute_stream << "Execute Stage| ";
            execute_buffer_data new_execute_buffer;
            decode_buffer_control_signals nop_control_signals;
            if (decodeBufferData.control_signals==nop_control_signals){//if control signals are 0, do nothing (NOP), pass forward
                execute_stream << "NOP Instruction"; 
                new_execute_buffer.control_signals={0,0,0,0,0};

                clock_tick.arrive_and_wait();//falling edge (done reading)
                
                execute_lock.lock();
                execute_buff.data = new_execute_buffer;
                execute_lock.unlock();
                
                debug_msg(execute_stream);
                continue;
            }
            int alu_control = decodeBufferData.alu_control;
            int immediate = decodeBufferData.immediate;
            dependency rs1_dependency = decodeBufferData.dependencies[0];
            dependency rs2_dependency = decodeBufferData.dependencies[1];
            execute_stream << rs1_dependency.buffer_index*rs1_dependency.is_dependent*(rs1_dependency.stage_needed==2) << ", " << rs2_dependency.buffer_index*rs2_dependency.is_dependent*(rs2_dependency.stage_needed==2) << " | ";
            int reg1_data = (rs1_dependency.stage_needed==2 && rs1_dependency.is_dependent)? get_buffer_data(rs1_dependency.buffer_index,decodeBufferData.reg1) : decodeBufferData.reg1_data;
            int reg2_data = (rs2_dependency.stage_needed==2 && rs2_dependency.is_dependent)? get_buffer_data(rs2_dependency.buffer_index,decodeBufferData.reg2) : decodeBufferData.reg2_data;
        
            int input2 = decodeBufferData.control_signals.ALUSrc ? immediate : reg2_data; //ALU_Src mux
            ALU_Op alu_operation = ALU_Control(decodeBufferData.control_signals.ALUOp, alu_control);
            int alu_result;
            switch(alu_operation){
                case ADD://also for nop
                    alu_result = reg1_data + input2;
                    execute_stream << std::dec << alu_result << " = " << reg1_data << " + " << input2;
                    break;
                case SUB:
                    alu_result = reg1_data - input2;
                    execute_stream << std::dec << alu_result << " = " << reg1_data << " - " << input2;
                    break;
                case AND:
                    alu_result = reg1_data & input2;
                    execute_stream << std::dec << alu_result << " = " << reg1_data << " & " << input2;
                    break;
                case OR:
                    alu_result = reg1_data | input2;
                    execute_stream << std::dec << alu_result << " = " << reg1_data << " | " << input2;
                    break;
                default:
                    throw std::runtime_error("Undefined ALU Behavior for alu_control "+std::bitset<4>(alu_control).to_string());
            }
            
            clock_tick.arrive_and_wait();

            //update execute buffer
            new_execute_buffer.control_signals = {decodeBufferData.control_signals.jump, 
                                            decodeBufferData.control_signals.RegWrite, 
                                            decodeBufferData.control_signals.MemToReg, 
                                            decodeBufferData.control_signals.MemRead, 
                                            decodeBufferData.control_signals.MemWrite};
            new_execute_buffer.next_pc = decodeBufferData.next_pc;
            new_execute_buffer.alu_result = decodeBufferData.control_signals.jump ? decodeBufferData.next_pc : alu_result;
            new_execute_buffer.write_register = decodeBufferData.write_register;
            new_execute_buffer.reg2 = decodeBufferData.reg2;
            new_execute_buffer.reg2_data = reg2_data;
            new_execute_buffer.dependencies[0] = decodeBufferData.dependencies[0];
            new_execute_buffer.dependencies[1] = decodeBufferData.dependencies[1];

            execute_lock.lock();
            execute_buff.data = new_execute_buffer;
            execute_lock.unlock();

            debug_msg(execute_stream);
        }
        clock_tick.arrive_and_drop();
        return;
    }

    void Mem(std::stop_token stop_token){
        std::unique_lock<std::mutex> execute_lock(execute_buff.get_mutex(),std::defer_lock);
        std::unique_lock<std::mutex> mem_lock(mem_buff.get_mutex(),std::defer_lock);
        std::ostringstream mem_stream;

        while(true){
            clock_tick.arrive_and_wait();//wait to start all threads in sync (rising edge)
            if(stop_token.stop_requested()){break;}
            
            execute_lock.lock();
            execute_buffer_data executeBufferData = execute_buff.data;
            execute_lock.unlock();

            mem_stream.str("");
            mem_stream << "Memory Stage | ";
            mem_buffer_data new_mem_buffer;
            execute_buffer_control_signals nop_control_signals;
            if (executeBufferData.control_signals==nop_control_signals){//if control signals are 0, do nothing (NOP), pass forward
                mem_stream << "NOP Instruction";
                new_mem_buffer.control_signals={0,0,0};

                clock_tick.arrive_and_wait();

                mem_lock.lock();
                mem_buff.data = new_mem_buffer;
                mem_lock.unlock();

                debug_msg(mem_stream);
                continue;
            }
            int alu_result = executeBufferData.alu_result;
            dependency rs2_dependency = executeBufferData.dependencies[1];
            int reg2_data = (rs2_dependency.stage_needed==3 && rs2_dependency.is_dependent)? get_buffer_data(rs2_dependency.buffer_index,executeBufferData.reg2) : executeBufferData.reg2_data;
            int next_pc = executeBufferData.next_pc;
            int write_reg = executeBufferData.write_register;
    
            int address = alu_result/sizeof(int); //data memory is composed of ints so divide by sizeof(int)
            int mem_read_data=executeBufferData.control_signals.jump ? executeBufferData.next_pc : alu_result; //jump mux
            if(executeBufferData.control_signals.MemRead){
                mem_read_data = dmem_access(true,address);
                mem_stream << "Read memory at 0x" << std::hex << alu_result << " = 0x" << std::hex << mem_read_data;
            }
            clock_tick.arrive_and_wait(); //falling edge(done reading)
            
            if(executeBufferData.control_signals.MemWrite){
                dmem_access(false,address,reg2_data);//output ignored cause write
                mem_stream << "memory 0x" << std::hex << alu_result << " is modified to 0x" << std::hex << reg2_data;
            }
            if(!executeBufferData.control_signals.MemRead && !executeBufferData.control_signals.MemWrite){
                mem_stream << "Did not use Memory";
            }
            
            //update mem buffer
            new_mem_buffer.control_signals = {executeBufferData.control_signals.jump, 
                                        executeBufferData.control_signals.RegWrite, 
                                        executeBufferData.control_signals.MemToReg};
            new_mem_buffer.next_pc = next_pc;
            new_mem_buffer.alu_result = alu_result;
            new_mem_buffer.write_register = write_reg;
            new_mem_buffer.mem_read_data = mem_read_data;
            
            mem_lock.lock();
            mem_buff.data = new_mem_buffer;
            mem_lock.unlock();
            debug_msg(mem_stream);
        }
        clock_tick.arrive_and_drop();
        return;
    }

    void Write(std::stop_token stop_token){
        std::unique_lock<std::mutex> mem_lock(mem_buff.get_mutex(),std::defer_lock);
        std::ostringstream write_stream;
        while(true){
            clock_tick.arrive_and_wait();//wait to start all threads in sync (rising edge)
            if(stop_token.stop_requested()){break;}

            mem_lock.lock();
            mem_buffer_data memBufferData = mem_buff.data;
            mem_lock.unlock();

            write_stream.str("");
            write_stream << "Write Stage  | ";
            mem_buffer_control_signals nop_control_signals;
            if (memBufferData.control_signals==nop_control_signals){//if control signals are 0, do nothing (NOP)
                write_stream << "NOP Instruction";
                clock_tick.arrive_and_wait(); //falling edge
                //nothing to write
                debug_msg(write_stream);
                continue;
            }
            int mem_read_data = memBufferData.mem_read_data;
            int alu_result = memBufferData.alu_result;
            int next_pc = memBufferData.next_pc;
            int write_reg = memBufferData.write_register;
    
            int write_data = memBufferData.control_signals.MemToReg ? mem_read_data : alu_result;     //write back mux
            write_data = memBufferData.control_signals.jump ? next_pc : write_data; //jump mux
            
            clock_tick.arrive_and_wait(); // falling edge (reading done)
            if(memBufferData.control_signals.RegWrite){
                reg_file_access(false,write_reg,write_data);//ignore output cause write
                write_stream << "x" << std::dec << write_reg << " is modified to 0x" << std::hex << write_data;
            }
            else{write_stream << "Did not write";}
            debug_msg(write_stream);
        }
        clock_tick.arrive_and_drop();
        return;
    }

    void debug_msg(const std::ostringstream& oss){
        std::lock_guard<std::mutex> lock(print_mtx);
        std::cout << oss.str() << std::endl;
    }
};

#endif